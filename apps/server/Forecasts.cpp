#include "Service.h"
#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>
#include <QSet>
#include <QTimeZone>
#include <cmath>
#include <csignal>
#include <unistd.h>

namespace {
void signalForecast(QProcess *process, int signal) {
  // uv and its Python child share the session created below.
  if (process->processId() > 0) ::kill(-pid_t(process->processId()), signal);
}
} // namespace

void Service::stopForecast() {
  if (!forecastProcess_) return;
  auto *process = forecastProcess_;
  process->disconnect(this);
  const auto group = pid_t(process->processId());
  signalForecast(process, SIGTERM);
  process->waitForFinished(3000);
  // The launcher can exit while its child is still running.
  if (group > 0) ::kill(-group, SIGKILL);
  if (process->state() != QProcess::NotRunning) process->waitForFinished(1000);
  forecastProcess_ = nullptr;
}
QJsonObject Service::forecasts() const {
  if (!forecastData_.isEmpty()) return forecastData_;
  return {{"generatedAt", ""},
          {"modelVersion", ""},
          {"source", "预测尚未生成"},
          {"stations", QJsonArray{}}};
}
void Service::runForecast() {
  if (forecastProcess_) return;
  try {
    QJsonArray stationRows;
    for (auto value : stations()) {
      auto station = value.toObject();
      station.insert("chargers",
                     chargers({{"stationId", station.value("id")}}));
      stationRows.append(station);
    }
    auto sessions = db_.rows(
      "SELECT station_id AS stationId,charger_id AS chargerId,started_at AS "
      "startedAt,ended_at AS endedAt,energy_wh / 1000.0 AS energyKwh FROM "
      "orders WHERE status IN ('paid','pending_payment') AND started_at<>'' "
      "AND ended_at<>'' ORDER BY started_at");
    QSaveFile input(directory_ + "/forecast-input.json");
    const auto origin = QDateTime::currentDateTimeUtc();
    if (!input.open(QIODevice::WriteOnly))
      throw BusinessError("IO_ERROR", "无法写入预测输入");
    input.write(QJsonDocument(QJsonObject{{"generatedAt",
                                           origin.toString(Qt::ISODateWithMs)},
                                          {"stations", stationRows},
                                          {"sessions", sessions},
                                          {"timeScale", scale_}})
                  .toJson(QJsonDocument::Compact));
    if (!input.commit()) throw BusinessError("IO_ERROR", "无法提交预测输入");
    const auto outputPath = directory_ + "/forecast-output.json";
    ensure(!QFile::exists(outputPath) || QFile::remove(outputPath), "IO_ERROR",
           "无法清理上次预测输出");
    forecastError_.clear();
    auto *process = new QProcess(this);
    forecastProcess_ = process;
    process->setChildProcessModifier([] {
      ::setsid();
    });
    process->setWorkingDirectory(sourceRoot_);
    process->setProcessChannelMode(QProcess::MergedChannels);
    process->setProgram(qEnvironmentVariable("CHARGING_UV", "uv"));
    process->setArguments({"run", "--project", sourceRoot_ + "/ml", "python",
                           "-m", "ml.service", "--input",
                           directory_ + "/forecast-input.json", "--output",
                           directory_ + "/forecast-output.json"});
    auto *timeout = new QTimer(process);
    timeout->setSingleShot(true);
    timeout->start(180000);
    connect(timeout, &QTimer::timeout, process, [process] {
      signalForecast(process, SIGKILL);
    });
    connect(process, &QProcess::errorOccurred, this,
            [this, process](QProcess::ProcessError error) {
              if (error == QProcess::FailedToStart) {
                forecastError_ = "无法启动 uv/Python，请完成机器学习环境安装";
                forecastProcess_ = nullptr;
                process->deleteLater();
              }
            });
    connect(process, &QProcess::finished, this,
            [this, process, origin](int exitCode, QProcess::ExitStatus status) {
              const auto output = process->readAll();
              forecastProcess_ = nullptr;
              process->deleteLater();
              if (exitCode != 0 || status != QProcess::NormalExit) {
                forecastError_ = "预测任务失败，请查看服务日志";
                qWarning().noquote() << "Forecast failed:"
                                     << QString::fromUtf8(output).right(3000);
                return;
              }
              try {
                importForecast(origin);
              } catch (const BusinessError &e) {
                forecastError_ = e.message;
              }
            });
    process->start();
  } catch (const std::exception &e) {
    forecastError_ = QString::fromUtf8(e.what());
  }
}

void Service::importForecast(const QDateTime &origin) {
  QFile file(directory_ + "/forecast-output.json");
  ensure(file.open(QIODevice::ReadOnly), "FORECAST_ERROR", "预测结果文件缺失");
  auto result = QJsonDocument::fromJson(file.readAll()).object();
  ensure(result.value("stations").isArray()
           && !result.value("generatedAt").toString().isEmpty(),
         "FORECAST_ERROR", "预测结果格式不完整");
  const auto generated = QDateTime::fromString(
                           result.value("generatedAt").toString(), Qt::ISODate)
                           .toUTC();
  ensure(generated.isValid() && std::abs(generated.msecsTo(origin)) < 1000,
         "FORECAST_ERROR", "预测生成时间与本次任务不一致");
  const auto hourOne = QDateTime(generated.date(),
                                 QTime(generated.time().hour(), 0),
                                 QTimeZone::utc())
                         .addSecs(3600);
  QSet<qint64> seenStations;
  for (auto value : result.value("stations").toArray()) {
    auto station = value.toObject();
    auto sid = station.value("stationId").toInteger();
    ensure(!seenStations.contains(sid)
             && !db_.row("SELECT id FROM stations WHERE id=?", {sid}).isEmpty(),
           "FORECAST_ERROR", "预测站点无效或重复");
    seenStations.insert(sid);
    auto hours = station.value("hours").toArray();
    ensure(hours.size() == 24, "FORECAST_ERROR", "预测缺少24个逐小时时距");
    double chargerTotals[24]{};
    QSet<qint64> seenChargers;
    for (auto item : station.value("chargers").toArray()) {
      auto charger = item.toObject();
      auto cid = charger.value("chargerId").toInteger();
      const auto device = db_.row(
        "SELECT power_kw FROM chargers WHERE id=? AND station_id=?",
        {cid, sid});
      ensure(!seenChargers.contains(cid) && !device.isEmpty(), "FORECAST_ERROR",
             "预测电桩无效或重复");
      seenChargers.insert(cid);
      auto points = charger.value("hours").toArray();
      ensure(points.size() == 24, "FORECAST_ERROR", "电桩预测缺少时距");
      for (int i = 0; i < 24; ++i) {
        auto point = points.at(i).toObject();
        auto load = point.value("loadKw");
        ensure(load.isDouble() && std::isfinite(load.toDouble())
                 && load.toDouble() >= 0
                 && load.toDouble()
                      <= device.value("power_kw").toDouble() + 0.001
                 && QDateTime::fromString(point.value("time").toString(),
                                          Qt::ISODate)
                      == hourOne.addSecs(i * 3600)
                 && point.value("hour").toInt() == i + 1,
               "FORECAST_ERROR", "电桩预测数值无效");
        chargerTotals[i] += load.toDouble();
      }
    }
    auto capacity = db_
                      .row("SELECT COUNT(*) AS n FROM chargers WHERE "
                           "station_id=?",
                           {sid})
                      .value("n")
                      .toInt();
    ensure(seenChargers.size() == capacity, "FORECAST_ERROR", "电桩预测不完整");
    for (int i = 0; i < 24; ++i) {
      auto point = hours.at(i).toObject();
      auto load = point.value("loadKw");
      auto target = QDateTime::fromString(point.value("time").toString(),
                                          Qt::ISODate);
      auto available = point.value("availableChargers").toDouble(-1);
      ensure(load.isDouble() && std::isfinite(load.toDouble())
               && load.toDouble() >= 0 && point.value("hour").toInt() == i + 1
               && target == hourOne.addSecs(i * 3600),
             "FORECAST_ERROR", "站点预测时距或数值无效");
      ensure(available >= 0 && available <= capacity
               && std::floor(available) == available
               && std::abs(chargerTotals[i] - load.toDouble()) < 0.1,
             "FORECAST_ERROR", "预测空闲数或站桩总负荷不一致");
    }
  }
  ensure(seenStations.size()
           == db_.row("SELECT COUNT(*) AS n FROM stations").value("n").toInt(),
         "FORECAST_ERROR", "站点预测不完整");
  db_.execute(
    "INSERT INTO forecasts(id,payload) VALUES(1,?) ON "
    "CONFLICT(id) DO UPDATE SET payload=excluded.payload",
    {QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact))});
  forecastData_ = result;
  forecastRunAt_ = utcNow();
  forecastError_.clear();
  audit({"system", 0, {}}, "负荷预测更新", "平台",
        result.value("source").toString());
}
