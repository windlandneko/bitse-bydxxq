#include "AdminUi.h"
#include "AdminWindowState.h"

using namespace adminui;

void AdminMainWindow::Impl::buildForecasts() {
  QVBoxLayout *layout;
  page("站点与电桩负荷预测", &layout);
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
  layout->addWidget(new QLabel("预测为运营参考；高峰时段请提前检查设备、安排值"
                               "守。时间按当前计算机时区显示。"));
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
        forecastState->setText(
          "预测任务正在后台运行，完成后将自动刷新。可继续管理其他业务。");
        forecastPoll->start();
      },
      true,
      [this] {
        forecastRequestPending = false;
        runForecast->setEnabled(!forecastRunning);
        forecastState->setText("提交失败，可检查服务连接后重试。");
      });
  });
}

QJsonObject AdminMainWindow::Impl::horizon(const QJsonArray &hours, int hour) {
  for (const auto &value : hours) {
    if (value.toObject()["hour"].toInt() == hour) return value.toObject();
  }
  return {};
}

QString AdminMainWindow::Impl::peaks(const QJsonArray &hours) {
  QStringList periods;
  for (const auto &value : hours) {
    const auto item = value.toObject();
    if (!item["isPeak"].toBool()) continue;
    auto dateTime = QDateTime::fromString(item["time"].toString(), Qt::ISODate);
    periods << (dateTime.isValid()
                  ? dateTime.toLocalTime().toString("MM-dd HH:mm")
                  : QString("+%1h").arg(item["hour"].toInt()));
  }
  return periods.isEmpty() ? "无高峰预警" : periods.join("、");
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
        forecastMeta->setText(
          "当前没有预测结果。点击“更新未来 24 小时预测”生成站级与桩级预测。");
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
      forecastWarnings->setText(QString("当前展示 %1 个电站、%2 个电桩；未来 "
                                        "24 小时有 %3 个电站出现高峰预警。")
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
        forecastState->setText(
          "预测任务正在后台运行，完成后自动刷新。可继续操作其他页面。");
        if (!forecastPoll->isActive()) forecastPoll->start();
      } else {
        forecastPoll->stop();
        const auto lastError = status["lastError"].toString();
        if (!lastError.isEmpty()) {
          forecastState->setText("最近一次预测任务失败：" + lastError);
        } else {
          forecastState->setText(status["lastRunAt"].toString().isEmpty()
                                   ? "尚未运行预测任务"
                                   : "最近任务完成时间："
                                       + timeText(status["lastRunAt"]));
        }
        if (wasRunning) {
          refreshForecasts(false);
          w->statusBar()->showMessage(lastError.isEmpty()
                                        ? "负荷预测已更新"
                                        : "预测任务失败，请查看预测页面",
                                      10000);
        }
      }
    },
    interactive);
}
