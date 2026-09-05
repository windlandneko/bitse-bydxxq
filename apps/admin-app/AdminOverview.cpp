#include "AdminUi.h"
#include "AdminWindowState.h"

using namespace adminui;

void AdminMainWindow::Impl::buildOverview() {
  QVBoxLayout *layout;
  page("运营概览", &layout);
  auto *metrics = new QHBoxLayout;
  todayRevenue = metric("今日营收", metrics);
  monthRevenue = metric("本月营收", metrics);
  totalRevenue = metric("总营收", metrics);
  todayOrders = metric("今日成交订单", metrics);
  layout->addLayout(metrics);
  auto *row = new QHBoxLayout;
  row->addWidget(
    new QLabel("营收趋势（以已结算订单计，业务时区：中国标准时间）"));
  row->addStretch();
  trendDays = new QComboBox;
  trendDays->setObjectName("revenueTrendDays");
  trendDays->addItem("近 7 日", 7);
  trendDays->addItem("近 30 日", 30);
  row->addWidget(trendDays);
  QObject::connect(trendDays, qOverload<int>(&QComboBox::currentIndexChanged),
                   w, [this] {
                     if (loggedIn) refreshOverview(true);
                   });
  layout->addLayout(row);
  revenueChart = new QChartView(new QChart);
  revenueChart->setObjectName("revenueChart");
  revenueChart->setRenderHint(QPainter::Antialiasing);
  revenueChart->setMinimumHeight(240);
  layout->addWidget(revenueChart, 2);
  auto *split = new QSplitter;
  auto *statusBox = new QGroupBox("当前电桩状态分布");
  auto *statusLayout = new QVBoxLayout(statusBox);
  statusTable = table({"状态", "数量", "占比 (%)"}, "statusTable");
  statusLayout->addWidget(statusTable);
  split->addWidget(statusBox);
  auto *trendBox = new QGroupBox("每日营收与订单数");
  auto *trendLayout = new QVBoxLayout(trendBox);
  trendTable = table({"日期", "营收 (元)", "订单数"}, "trendTable");
  trendLayout->addWidget(trendTable);
  split->addWidget(trendBox);
  layout->addWidget(split, 1);
}

void AdminMainWindow::Impl::refreshOverview(bool interactive) {
  read(
    "admin.overview", {{"days", trendDays->currentData().toInt()}},
    [this](QJsonValue data) {
      const auto o = data.toObject();
      todayRevenue->setText(money(o["todayRevenueCents"]));
      monthRevenue->setText(money(o["monthRevenueCents"]));
      totalRevenue->setText(money(o["totalRevenueCents"]));
      todayOrders->setText(QString::number(o["todayOrders"].toInt()));
      fill(statusTable, o["statusCounts"].toArray(),
           [](const QJsonObject &item) -> QVariantList {
             return {item["label"].toString(state(item["status"].toString())),
                     item["count"].toInt(), number(item["percent"])};
           });
      const auto trend = o["revenueTrend"].toArray();
      fill(trendTable, trend, [](const QJsonObject &item) -> QVariantList {
        return {item["date"].toString(), money(item["revenueCents"]),
                item["orderCount"].toInt()};
      });
      auto *chart = new QChart;
      chart->setTitle(
        QString("近 %1 日营收趋势").arg(trendDays->currentData().toInt()));
      chart->legend()->hide();
      auto *series = new QLineSeries(chart);
      series->setName("营收 (元)");
      series->setPointsVisible(true);
      double maximum = 1;
      qint64 first = 0, last = 0;
      for (const auto &value : trend) {
        const auto item = value.toObject();
        const QDate date = QDate::fromString(item["date"].toString(),
                                             "yyyy-MM-dd");
        if (!date.isValid()) continue;
        const auto timestamp = QDateTime(date, QTime(12, 0))
                                 .toMSecsSinceEpoch();
        const auto amount = item["revenueCents"].toDouble() / 100.0;
        series->append(timestamp, amount);
        if (!first || timestamp < first) first = timestamp;
        if (timestamp > last) last = timestamp;
        maximum = qMax(maximum, amount);
      }
      chart->addSeries(series);
      auto *x = new QDateTimeAxis(chart);
      x->setFormat("MM-dd");
      x->setTickCount(trendDays->currentData().toInt() == 7 ? 7 : 6);
      x->setTitleText("日期");
      if (!first)
        first = QDateTime::currentDateTime().addDays(-6).toMSecsSinceEpoch();
      if (last <= first) last = first + 86400000;
      x->setRange(QDateTime::fromMSecsSinceEpoch(first),
                  QDateTime::fromMSecsSinceEpoch(last));
      auto *y = new QValueAxis(chart);
      y->setTitleText("营收 (元)");
      y->setLabelFormat("%.2f");
      y->setRange(0, maximum * 1.15);
      chart->addAxis(x, Qt::AlignBottom);
      chart->addAxis(y, Qt::AlignLeft);
      series->attachAxis(x);
      series->attachAxis(y);
      auto *old = revenueChart->chart();
      revenueChart->setChart(chart);
      delete old;
    },
    interactive);
}
