#include "Service.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QNetworkReply>
#include <QSaveFile>
#include <QSet>
#include <QTimeZone>
#include <QUrlQuery>
#include <QUuid>
#include <cmath>
#ifdef Q_OS_UNIX
#include <csignal>
#include <unistd.h>
#endif

namespace {
void signalForecast(QProcess *process, bool force) {
  if (process->processId() <= 0) return;
#ifdef Q_OS_UNIX
  // Terminate uv and its Python child together.
  ::kill(-pid_t(process->processId()), force ? SIGKILL : SIGTERM);
#else
  if (force)
    process->kill();
  else
    process->terminate();
#endif
}
} // namespace

void ensure(bool condition, const QString &code, const QString &message) {
  if (!condition) throw BusinessError(code, message);
}
QString requiredText(const QJsonObject &p, const QString &key, int maxLength) {
  auto value = p.value(key);
  ensure(value.isString(), "VALIDATION_ERROR", "缺少文本字段：" + key);
  QString text = value.toString().trimmed();
  ensure(!text.isEmpty() && text.size() <= maxLength, "VALIDATION_ERROR",
         key + "不能为空或超过长度限制");
  return text;
}
qint64 integer(const QJsonObject &p, const QString &key, qint64 min,
               qint64 max) {
  auto v = p.value(key);
  double n = v.toDouble(-1);
  ensure(v.isDouble() && std::isfinite(n) && std::floor(n) == n && n >= min
           && n <= max,
         "VALIDATION_ERROR", key + "必须是有效范围内的整数");
  return qint64(n);
}
Service::Service(QString directory, QString sourceRoot, int scale, bool seed)
    : directory_(std::move(directory)), sourceRoot_(std::move(sourceRoot)),
      scale_(scale), seed_(seed) {}
void Service::shutdown() {
  for (auto *timer : findChildren<QTimer *>()) timer->stop();
  if (!forecastProcess_) return;
  auto *process = forecastProcess_;
  process->disconnect(this);
#ifdef Q_OS_UNIX
  const auto group = pid_t(process->processId());
#endif
  signalForecast(process, false);
  process->waitForFinished(3000);
#ifdef Q_OS_UNIX
  // Also reap an uncooperative child after its launcher has already exited.
  if (group > 0) ::kill(-group, SIGKILL);
#else
  process->kill();
#endif
  if (process->state() != QProcess::NotRunning) process->waitForFinished(1000);
  forecastProcess_ = nullptr;
}
void Service::initialize() {
  try {
    db_.open(directory_, seed_);
    directory_ = db_.directory();
    auto stored = db_.row("SELECT payload FROM forecasts WHERE id=1");
    forecastData_ = QJsonDocument::fromJson(
                      stored.value("payload").toString().toUtf8())
                      .object();
    network_ = new QNetworkAccessManager(this);
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, [this] {
      try {
        tick();
      } catch (const std::exception &e) {
        qWarning("Background state update failed: %s", e.what());
      }
    });
    timer_->start(1000);
    tick();
    if (qEnvironmentVariableIsSet("CHARGING_DISABLE_AUTO_FORECAST")) {
      emit ready();
      return;
    }
    QTimer::singleShot(1500, this, [this] {
      runForecast();
    });
    auto *forecastTimer = new QTimer(this);
    connect(forecastTimer, &QTimer::timeout, this, &Service::runForecast);
    forecastTimer->start(15 * 60 * 1000);
    emit ready();
  } catch (const std::exception &e) {
    emit fatal(QString::fromUtf8(e.what()));
  }
}
Principal Service::authorize(const QString &action, const QString &token) {
  const QStringList publicActions = {
    "health",           "dashboard",        "user.login",    "admin.login",
    "location.presets", "location.geocode", "stations.list", "stations.detail"};
  if (publicActions.contains(action)) return {};
  ensure(tokens_.contains(token), "UNAUTHENTICATED", "登录已失效，请重新登录");
  auto actor = tokens_.value(token);
  ensure(actor.expires > QDateTime::currentDateTimeUtc(), "UNAUTHENTICATED",
         "登录已过期，请重新登录");
  if (action.startsWith("admin.") || action == "forecasts.run"
      || action == "forecasts.status")
    ensure(actor.role == "admin", "FORBIDDEN", "需要管理员权限");
  if (action.startsWith("user.") || action.startsWith("wallet.")
      || action.startsWith("orders."))
    ensure(actor.role == "user", "FORBIDDEN", "需要车主账户登录");
  return actor;
}
void Service::request(quint64 id, QJsonObject input, QString token) {
  try {
    QString action = requiredText(input, "action", 80);
    ensure(!input.contains("params") || input.value("params").isObject(),
           "VALIDATION_ERROR", "params必须是对象");
    auto p = input.value("params").toObject();
    auto actor = authorize(action, token);
    if (action == "location.geocode") {
      geocode(id, p);
      return;
    }
    if (action == "auth.logout") {
      tokens_.remove(token);
      emit replied(id, success());
      return;
    }
    bool idempotent = action == "wallet.recharge" || action == "orders.reserve";
    if (idempotent) {
      auto key = requiredText(p, "idempotencyKey", 100);
      auto clean = p;
      clean.remove("idempotencyKey");
      auto payload = QString::fromUtf8(
        QJsonDocument(clean).toJson(QJsonDocument::Compact));
      auto principal = actor.role + ":" + QString::number(actor.id);
      Transaction tx(db_);
      auto old = db_.row("SELECT payload,response FROM requests WHERE "
                         "principal=? AND action=? AND key=?",
                         {principal, action, key});
      if (!old.isEmpty()) {
        ensure(old.value("payload").toString() == payload,
               "IDEMPOTENCY_CONFLICT",
               "同一请求编号对应了不同的内容，请重新操作");
        tx.commit();
        emit replied(
          id, QJsonDocument::fromJson(old.value("response").toString().toUtf8())
                .object());
        return;
      }
      auto result = success(dispatch(action, p, actor));
      db_.execute("INSERT INTO "
                  "requests(principal,action,key,payload,response,created_at) "
                  "VALUES(?,?,?,?,?,?)",
                  {principal, action, key, payload,
                   QString::fromUtf8(
                     QJsonDocument(result).toJson(QJsonDocument::Compact)),
                   utcNow()});
      tx.commit();
      emit replied(id, result);
    } else
      emit replied(id, success(dispatch(action, p, actor)));
  } catch (const BusinessError &e) {
    if (e.code == "DATABASE_ERROR") {
      qWarning("Database request failed: %s", e.what());
      emit replied(id, failure(e.code, "数据操作未完成，请稍后重试"));
    } else
      emit replied(id, failure(e.code, e.message));
  } catch (const std::exception &e) {
    qWarning("Request failed: %s", e.what());
    emit replied(id, failure("INTERNAL_ERROR", "服务处理失败，请稍后重试"));
  }
}
QJsonValue Service::dispatch(const QString &action, const QJsonObject &p,
                             const Principal &actor) {
  if (action == "health")
    return QJsonObject{{"status", "ok"},
                       {"schemaVersion", 2},
                       {"time", utcNow()},
                       {"timeScale", scale_}};
  if (action == "dashboard") return dashboard();
  if (action == "forecasts.list") {
    auto result = forecasts();
    if (p.contains("stationId")) {
      auto id = integer(p, "stationId");
      QJsonArray filtered;
      for (auto value : result.value("stations").toArray())
        if (value.toObject().value("stationId").toInteger() == id)
          filtered.append(value);
      result.insert("stations", filtered);
    }
    return result;
  }
  if (action == "forecasts.run") {
    runForecast();
    return QJsonObject{{"running", forecastProcess_ != nullptr}};
  }
  if (action == "forecasts.status")
    return QJsonObject{{"running", forecastProcess_ != nullptr},
                       {"lastError", forecastError_},
                       {"lastRunAt", forecastRunAt_}};
  if (action == "admin.overview") {
    int days = p.value("days").toInt(7);
    ensure(days == 7 || days == 30, "VALIDATION_ERROR", "仅支持7日或30日");
    return overview(days);
  }
  if (action == "admin.logs")
    return db_.rows("SELECT id,action,target,detail,created_at AS createdAt "
                    "FROM audit_logs ORDER BY id DESC LIMIT 200");
  if (action.startsWith("orders.") || action == "admin.orders")
    return orderAction(action, p, actor);
  if (action.startsWith("stations.") || action.startsWith("location.")
      || action.startsWith("admin.station")
      || action.startsWith("admin.charger"))
    return stationAction(action, p, actor);
  return accountAction(action, p, actor);
}
void Service::audit(const Principal &actor, const QString &action,
                    const QString &target, const QString &detail) {
  db_.execute("INSERT INTO audit_logs(actor,action,target,detail,created_at) "
              "VALUES(?,?,?,?,?)",
              {actor.role + ":" + QString::number(actor.id), action, target,
               detail, utcNow()});
}
void Service::requireActive(const Principal &actor) {
  ensure(user(actor.id).value("status").toString() == "active",
         "ACCOUNT_FROZEN", "账号已冻结，暂时无法开始新的充电");
}
void Service::geocode(quint64 id, const QJsonObject &p) {
  auto address = requiredText(p, "address", 200);
  QString key = qEnvironmentVariable("TENCENT_MAP_KEY"),
          secret = qEnvironmentVariable("TENCENT_MAP_SECRET");
  QFile config(directory_ + "/map-settings.json");
  if (config.open(QIODevice::ReadOnly)) {
    auto settings = QJsonDocument::fromJson(config.readAll()).object();
    if (key.isEmpty()) key = settings.value("key").toString();
    if (secret.isEmpty()) secret = settings.value("secret").toString();
  }
  ensure(!key.isEmpty(), "MAP_NOT_CONFIGURED",
         "地图服务尚未配置，请先选择预设区域定位");
  // Tencent signs the decoded query string; the URL is encoded for transport
  // afterwards.
  QString path = "/ws/geocoder/v1/";
  QString query = "address=" + address + "&key=" + key;
  QUrl url("https://apis.map.qq.com" + path);
  QUrlQuery queryItems;
  queryItems.addQueryItem(
    "address", QString::fromLatin1(QUrl::toPercentEncoding(address)));
  queryItems.addQueryItem("key",
                          QString::fromLatin1(QUrl::toPercentEncoding(key)));
  if (!secret.isEmpty())
    queryItems.addQueryItem(
      "sig", QString::fromLatin1(
               QCryptographicHash::hash((path + "?" + query + secret).toUtf8(),
                                        QCryptographicHash::Md5)
                 .toHex()));
  url.setQuery(queryItems);
  QNetworkRequest request(url);
  request.setTransferTimeout(10000);
  request.setRawHeader("x-legacy-url-decode", "no");
  auto *reply = network_->get(request);
  connect(reply, &QNetworkReply::finished, this, [this, reply, id, address] {
    auto networkError = reply->error();
    auto response = QJsonDocument::fromJson(reply->readAll()).object();
    reply->deleteLater();
    if (networkError != QNetworkReply::NoError) {
      emit replied(id, failure("MAP_NETWORK_ERROR",
                               "地图服务暂时无法连接，请重试或选择预设区域"));
      return;
    }
    if (response.value("status").toInt(-1) == 121) {
      emit replied(
        id, failure(
              "MAP_QUOTA_EXCEEDED",
              "腾讯地图今日调用额度已用完，请选择预设区域或待额度恢复后重试"));
      return;
    }
    if (response.value("status").toInt(-1) != 0) {
      emit replied(
        id, failure("MAP_ERROR",
                    "地址解析失败（地图状态码"
                      + QString::number(response.value("status").toInt(-1))
                      + "），请检查地址或地图配置"));
      return;
    }
    auto location = response.value("result")
                      .toObject()
                      .value("location")
                      .toObject();
    if (!location.value("lat").isDouble()
        || !location.value("lng").isDouble()) {
      emit replied(id, failure("MAP_ERROR", "地图返回了无效坐标"));
      return;
    }
    emit replied(id,
                 success(QJsonObject{{"name", address},
                                     {"latitude", location.value("lat")},
                                     {"longitude", location.value("lng")}}));
  });
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
#ifdef Q_OS_UNIX
    process->setChildProcessModifier([] {
      ::setsid();
    });
#endif
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
      signalForecast(process, true);
    });
    connect(process, &QProcess::errorOccurred, this,
            [this, process](QProcess::ProcessError error) {
              if (error == QProcess::FailedToStart) {
                forecastError_ = "无法启动 uv/Python，请完成机器学习环境安装";
                forecastProcess_ = nullptr;
                process->deleteLater();
              }
            });
    connect(
      process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
      [this, process, origin](int exitCode, QProcess::ExitStatus status) {
        const auto output = process->readAll();
        forecastProcess_ = nullptr;
        process->deleteLater();
        if (exitCode != 0 || status != QProcess::NormalExit) {
          forecastError_ = "预测任务失败，请查看服务日志";
          qWarning().noquote()
            << "Forecast failed:" << QString::fromUtf8(output).right(3000);
          return;
        }
        try {
          QFile file(directory_ + "/forecast-output.json");
          ensure(file.open(QIODevice::ReadOnly), "FORECAST_ERROR",
                 "预测结果文件缺失");
          auto result = QJsonDocument::fromJson(file.readAll()).object();
          ensure(result.value("stations").isArray()
                   && !result.value("generatedAt").toString().isEmpty(),
                 "FORECAST_ERROR", "预测结果格式不完整");
          const auto generated = QDateTime::fromString(
                                   result.value("generatedAt").toString(),
                                   Qt::ISODate)
                                   .toUTC();
          ensure(generated.isValid()
                   && std::abs(generated.msecsTo(origin)) < 1000,
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
                     && !db_.row("SELECT id FROM stations WHERE id=?", {sid})
                           .isEmpty(),
                   "FORECAST_ERROR", "预测站点无效或重复");
            seenStations.insert(sid);
            auto hours = station.value("hours").toArray();
            ensure(hours.size() == 24, "FORECAST_ERROR",
                   "预测缺少24个逐小时时距");
            double chargerTotals[24]{};
            QSet<qint64> seenChargers;
            for (auto item : station.value("chargers").toArray()) {
              auto charger = item.toObject();
              auto cid = charger.value("chargerId").toInteger();
              const auto device = db_.row(
                "SELECT power_kw FROM chargers WHERE id=? AND station_id=?",
                {cid, sid});
              ensure(!seenChargers.contains(cid) && !device.isEmpty(),
                     "FORECAST_ERROR", "预测电桩无效或重复");
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
                         && QDateTime::fromString(
                              point.value("time").toString(), Qt::ISODate)
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
            ensure(seenChargers.size() == capacity, "FORECAST_ERROR",
                   "电桩预测不完整");
            for (int i = 0; i < 24; ++i) {
              auto point = hours.at(i).toObject();
              auto load = point.value("loadKw");
              auto target = QDateTime::fromString(
                point.value("time").toString(), Qt::ISODate);
              auto available = point.value("availableChargers").toDouble(-1);
              ensure(load.isDouble() && std::isfinite(load.toDouble())
                       && load.toDouble() >= 0
                       && point.value("hour").toInt() == i + 1
                       && target == hourOne.addSecs(i * 3600),
                     "FORECAST_ERROR", "站点预测时距或数值无效");
              ensure(available >= 0 && available <= capacity
                       && std::floor(available) == available
                       && std::abs(chargerTotals[i] - load.toDouble()) < 0.1,
                     "FORECAST_ERROR", "预测空闲数或站桩总负荷不一致");
            }
          }
          ensure(seenStations.size()
                   == db_.row("SELECT COUNT(*) AS n FROM stations")
                        .value("n")
                        .toInt(),
                 "FORECAST_ERROR", "站点预测不完整");
          db_.execute("INSERT INTO forecasts(id,payload) VALUES(1,?) ON "
                      "CONFLICT(id) DO UPDATE SET payload=excluded.payload",
                      {QString::fromUtf8(
                        QJsonDocument(result).toJson(QJsonDocument::Compact))});
          forecastData_ = result;
          forecastRunAt_ = utcNow();
          forecastError_.clear();
          audit({"system", 0, {}}, "负荷预测更新", "平台",
                result.value("source").toString());
        } catch (const BusinessError &e) {
          forecastError_ = e.message;
        }
      });
    process->start();
  } catch (const std::exception &e) {
    forecastError_ = QString::fromUtf8(e.what());
  }
}
