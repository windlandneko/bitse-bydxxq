#include "AdminUi.h"
#include "AdminWindowState.h"

#include <QComboBox>
#include <QDateTime>
#include <QLabel>
#include <QPushButton>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>

using namespace adminui;

namespace {
constexpr auto runningMessage = "预测任务运行中，完成后自动刷新。";

QJsonObject horizon(const QJsonArray &hours, int hour) {
  for (const auto &value : hours) {
    const auto item = value.toObject();
    if (item["hour"].toInt() == hour) return item;
  }
  return {};
}

QString peaks(const QJsonArray &hours) {
  QStringList periods;
  for (const auto &value : hours) {
    const auto item = value.toObject();
    if (!item["isPeak"].toBool()) continue;
    const auto dateTime = QDateTime::fromString(item["time"].toString(),
                                                Qt::ISODate);
    periods << (dateTime.isValid()
                  ? dateTime.toLocalTime().toString("MM-dd HH:mm")
                  : QString("+%1h").arg(item["hour"].toInt()));
  }
  return periods.isEmpty() ? "无高峰预警" : periods.join("、");
}
} // namespace

void AdminMainWindow::Impl::buildForecasts() {
  auto *layout = page("站点与电桩负荷预测");
  auto *row = new QHBoxLayout;
  forecastStation = new QComboBox;
  forecastStation->setObjectName("forecastStationFilter");
  forecastStation->addItem("全部电站", 0);
  row->addWidget(new QLabel("查看电站"));
  row->addWidget(forecastStation, 1);
  runForecast = button("更新未来 24 小时预测", row);
  runForecast->setObjectName("runForecastButton");
  layout->addLayout(row);
  forecastMeta = new QLabel("尚未加载预测数据");
  forecastMeta->setWordWrap(true);
  forecastMeta->setTextInteractionFlags(Qt::TextSelectableByMouse);
  layout->addWidget(forecastMeta);
  forecastState = new QLabel;
  forecastState->setWordWrap(true);
  layout->addWidget(forecastState);
  forecastWarnings = new QLabel;
  forecastWarnings->setWordWrap(true);
  layout->addWidget(forecastWarnings);
  auto *tabs = new QTabWidget;
  stationForecasts = table({"电站", "+1h 负荷 (kW)", "+1h 空闲桩",
                            "+6h 负荷 (kW)", "+6h 空闲桩", "+24h 负荷 (kW)",
                            "+24h 空闲桩", "高峰预警时段"},
                           "stationForecastTable");
  chargerForecasts = table({"电站", "电桩编号", "+1h 负荷 (kW)",
                            "+6h 负荷 (kW)", "+24h 负荷 (kW)", "高峰预警时段"},
                           "chargerForecastTable");
  tabs->addTab(stationForecasts, "站级负荷与空闲电桩");
  tabs->addTab(chargerForecasts, "桩级负荷");
  layout->addWidget(tabs, 1);
  layout->addWidget(
    new QLabel("高峰时段请提前安排电力调配与运维值守。时间按本机时区显示。"));
  QObject::connect(forecastStation,
                   qOverload<int>(&QComboBox::currentIndexChanged), w, [this] {
                     if (loggedIn) refreshForecasts(false);
                   });
  QObject::connect(runForecast, &QPushButton::clicked, w, [this] {
    if (forecastRunning || forecastRequestPending) return;
    forecastRequestPending = true;
    runForecast->setEnabled(false);
    forecastState->setText("正在提交预测任务…");
    call(
      w, "forecasts.run", {},
      [this](QJsonValue) {
        forecastRequestPending = false;
        forecastRunning = true;
        forecastState->setText(runningMessage);
        forecastPoll->start();
      },
      true,
      [this] {
        forecastRequestPending = false;
        runForecast->setEnabled(!forecastRunning);
        forecastState->setText("提交失败，请重试。");
      });
  });
}

void AdminMainWindow::Impl::refreshForecasts(bool interactive) {
  QJsonObject params;
  if (forecastStation->currentData().toInt() > 0)
    params["stationId"] = forecastStation->currentData().toInt();
  read(
    "forecasts.list", params,
    [this](QJsonValue data) {
      const auto output = data.toObject();
      const auto stations = output["stations"].toArray();
      if (stations.isEmpty()) {
        forecastMeta->setText("暂无预测结果，点击“更新未来 24 小时预测”生成。");
      } else {
        forecastMeta->setText(QString("生成时间：%1\n模型：%2 · 数据来源：%3")
                                .arg(timeText(output["generatedAt"]),
                                     output["modelVersion"].toString("未提供"),
                                     output["source"].toString("未提供")));
      }
      QJsonArray stationRows, chargerRows;
      int warningStations = 0;
      for (const auto &value : stations) {
        auto item = value.toObject();
        item["id"] = item["stationId"];
        stationRows.append(item);
        const auto hours = item["hours"].toArray();
        if (std::any_of(hours.begin(), hours.end(), [](const QJsonValue &hour) {
              return hour.toObject()["isPeak"].toBool();
            }))
          ++warningStations;
        for (const auto &charger : item["chargers"].toArray()) {
          auto row = charger.toObject();
          row["id"] = row["chargerId"];
          row["stationName"] = item["stationName"];
          chargerRows.append(row);
        }
      }
      fill(stationForecasts, stationRows,
           [](const QJsonObject &item) -> QVariantList {
             const auto hours = item["hours"].toArray();
             QVariantList row{item["stationName"].toString()};
             for (const auto h : {1, 6, 24}) {
               const auto forecast = horizon(hours, h);
               row << number(forecast["loadKw"], 2)
                   << number(forecast["availableChargers"], 0);
             }
             row << peaks(hours);
             return row;
           });
      fill(chargerForecasts, chargerRows,
           [](const QJsonObject &item) -> QVariantList {
             const auto hours = item["hours"].toArray();
             return {item["stationName"].toString(),
                     item["code"].toString(),
                     number(horizon(hours, 1)["loadKw"], 2),
                     number(horizon(hours, 6)["loadKw"], 2),
                     number(horizon(hours, 24)["loadKw"], 2),
                     peaks(hours)};
           });
      forecastWarnings->setText(
        QString("%1 个电站 · %2 个电桩 · 未来 24 小时高峰预警：%3 站")
          .arg(stationRows.size())
          .arg(chargerRows.size())
          .arg(warningStations));
    },
    interactive);
}

void AdminMainWindow::Impl::refreshForecastStatus(bool interactive) {
  read(
    "forecasts.status", {},
    [this](QJsonValue data) {
      const auto status = data.toObject();
      const bool wasRunning = forecastRunning;
      forecastRunning = status["running"].toBool();
      runForecast->setEnabled(!forecastRunning && !forecastRequestPending);
      if (forecastRunning) {
        forecastState->setText(runningMessage);
        if (!forecastPoll->isActive()) forecastPoll->start();
        return;
      }
      forecastPoll->stop();
      const auto lastError = status["lastError"].toString();
      if (!lastError.isEmpty()) {
        forecastState->setText("预测失败：" + lastError);
      } else if (status["lastRunAt"].toString().isEmpty()) {
        forecastState->setText("尚未运行预测任务");
      } else {
        forecastState->setText("上次完成：" + timeText(status["lastRunAt"]));
      }
      if (wasRunning) {
        refreshForecasts(false);
        w->statusBar()->showMessage(lastError.isEmpty()
                                      ? "负荷预测已更新"
                                      : "预测任务失败，请查看预测页面",
                                    10000);
      }
    },
    interactive);
}
