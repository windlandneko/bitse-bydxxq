#include "Service.h"
#include <QTimeZone>
#include <algorithm>
#include <map>

namespace {
QDateTime localTime(const QString &text) {
  return QDateTime::fromString(text, Qt::ISODate)
    .toTimeZone(QTimeZone("Asia/Shanghai"));
}
QDate localToday() {
  return QDateTime::currentDateTimeUtc()
    .toTimeZone(QTimeZone("Asia/Shanghai"))
    .date();
}
} // namespace
QJsonObject Service::overview(int days) {
  auto today = localToday();
  qint64 daily = 0, monthly = 0, total = 0, todayOrders = 0;
  QMap<QString, QPair<qint64, int>> grouped;
  for (auto value : db_.rows(
         "SELECT amount_cents,settled_at FROM orders WHERE status='paid'")) {
    auto item = value.toObject();
    auto date = localTime(item.value("settled_at").toString()).date();
    auto amount = item.value("amount_cents").toInteger();
    total += amount;
    if (date == today) {
      daily += amount;
      todayOrders++;
    }
    if (date.year() == today.year() && date.month() == today.month())
      monthly += amount;
    auto &point = grouped[date.toString(Qt::ISODate)];
    point.first += amount;
    point.second++;
  }
  QJsonArray trend;
  for (int i = days - 1; i >= 0; --i) {
    auto date = today.addDays(-i).toString(Qt::ISODate);
    trend.append(QJsonObject{{"date", date},
                             {"revenueCents", grouped[date].first},
                             {"orderCount", grouped[date].second}});
  }
  auto counts = db_.rows(
    "SELECT status,COUNT(*) AS count FROM chargers GROUP BY status");
  int used = 0, idle = 0, fault = 0;
  for (auto value : counts) {
    auto item = value.toObject();
    auto state = item.value("status").toString();
    if (state == "idle")
      idle += item.value("count").toInt();
    else if (state == "charging" || state == "reserved")
      used += item.value("count").toInt();
    else
      fault += item.value("count").toInt();
  }
  int n = std::max(1, used + idle + fault);
  QJsonArray distribution{QJsonObject{{"status", "charging"},
                                      {"label", "在用"},
                                      {"count", used},
                                      {"percent", 100.0 * used / n}},
                          QJsonObject{{"status", "idle"},
                                      {"label", "闲置"},
                                      {"count", idle},
                                      {"percent", 100.0 * idle / n}},
                          QJsonObject{{"status", "fault"},
                                      {"label", "故障 / 离线 / 重启"},
                                      {"count", fault},
                                      {"percent", 100.0 * fault / n}}};
  return {{"todayRevenueCents", daily},   {"monthRevenueCents", monthly},
          {"totalRevenueCents", total},   {"todayOrders", todayOrders},
          {"statusCounts", distribution}, {"revenueTrend", trend}};
}
QJsonObject Service::dashboard() {
  auto stats = overview(30);
  auto totals = db_.row(
    "SELECT COUNT(*) AS count,COALESCE(SUM(amount_cents),0)/100.0 AS "
    "revenue,COALESCE(SUM(energy_wh),0)/1000.0 AS energy FROM orders WHERE "
    "status='paid'");
  auto online = db_
                  .row("SELECT COUNT(*) AS count FROM chargers WHERE status IN "
                       "('idle','reserved','charging')")
                  .value("count");
  auto users = db_.row("SELECT COUNT(*) AS count FROM users").value("count");
  QJsonObject kpis{{"totalChargingCount", totals.value("count")},
                   {"totalRevenue", totals.value("revenue")},
                   {"totalEnergyKwh", totals.value("energy")},
                   {"onlineChargers", online},
                   {"registeredUsers", users}};
  QJsonArray trend, status;
  for (auto value : stats.value("revenueTrend").toArray()) {
    auto item = value.toObject();
    trend.append(
      QJsonObject{{"date", item.value("date")},
                  {"revenue", item.value("revenueCents").toDouble() / 100},
                  {"orderCount", item.value("orderCount")}});
  }
  const QMap<QString, QString> labels{
    {"idle", "闲置"},  {"reserved", "预约"}, {"charging", "充电中"},
    {"fault", "故障"}, {"offline", "离线"},  {"restarting", "重启中"}};
  for (auto it = labels.begin(); it != labels.end(); ++it)
    status.append(QJsonObject{
      {"key", it.key()},
      {"label", it.value()},
      {"value", db_
                  .row("SELECT COUNT(*) AS count FROM chargers WHERE status=?",
                       {it.key()})
                  .value("count")}});
  auto ranking = db_.rows(
    "SELECT s.id AS stationId,s.code AS stationCode,s.name AS "
    "stationName,COALESCE(SUM(o.energy_wh),0)/1000.0 AS "
    "energyKwh,COALESCE(SUM(o.amount_cents),0)/100.0 AS revenue FROM stations "
    "s LEFT JOIN orders o ON o.station_id=s.id AND o.status='paid' GROUP BY "
    "s.id ORDER BY energyKwh DESC LIMIT 10");
  auto types = db_.rows(
    "SELECT c.type,CASE c.type WHEN 'dc' THEN '快充 DC' ELSE '慢充 AC' END AS "
    "label,COUNT(DISTINCT c.id) AS count,COALESCE(SUM(o.energy_wh),0)/1000.0 "
    "AS energyKwh FROM chargers c LEFT JOIN orders o ON o.charger_id=c.id AND "
    "o.status='paid' GROUP BY c.type");
  double hours[7][24]{};
  for (auto value : db_.rows(
         "SELECT started_at,ended_at,energy_wh FROM orders WHERE status IN "
         "('paid','pending_payment') AND started_at<>'' AND ended_at<>''")) {
    auto item = value.toObject();
    auto start = localTime(item.value("started_at").toString()),
         end = localTime(item.value("ended_at").toString());
    qint64 duration = start.msecsTo(end);
    if (duration <= 0) continue;
    auto cursor = start;
    while (cursor < end) {
      auto boundary = QDateTime(cursor.date(), QTime(cursor.time().hour(), 0),
                                QTimeZone("Asia/Shanghai"))
                        .addSecs(3600);
      auto stop = std::min(boundary, end);
      hours[cursor.date().dayOfWeek() - 1]
           [cursor.time().hour()] += item.value("energy_wh").toDouble() / 1000.0
                                   * cursor.msecsTo(stop) / duration;
      cursor = stop;
    }
  }
  QJsonArray heatmap;
  for (int day = 0; day < 7; ++day)
    for (int hour = 0; hour < 24; ++hour)
      heatmap.append(QJsonObject{
        {"weekday", day}, {"hour", hour}, {"energyKwh", hours[day][hour]}});
  QMap<QString, QJsonObject> aggregate;
  for (auto value : forecastData_.value("stations").toArray())
    for (auto point : value.toObject().value("hours").toArray()) {
      auto hour = point.toObject();
      auto time = hour.value("time").toString();
      auto &item = aggregate[time];
      item.insert("time", time);
      item.insert("actualLoadKw", QJsonValue::Null);
      item.insert("predictedLoadKw", item.value("predictedLoadKw").toDouble()
                                       + hour.value("loadKw").toDouble());
      item.insert("availableChargers",
                  item.value("availableChargers").toInt()
                    + hour.value("availableChargers").toInt());
      item.insert("isPeak", item.value("isPeak").toBool()
                              || hour.value("isPeak").toBool());
    }
  QJsonArray forecast;
  for (auto item : aggregate) forecast.append(item);
  auto last = db_
                .row(
                  "SELECT created_at FROM audit_logs ORDER BY id DESC LIMIT 1")
                .value("created_at")
                .toString();
  auto now = utcNow();
  QJsonObject result{
    {"generatedAt", now},
    {"dataCutoff", now},
    {"source", "课程演示业务数据"},
    {"kpis", kpis},
    {"chargerStatus", status},
    {"stationRanking", ranking},
    {"revenueTrend", trend},
    {"hourlyHeatmap", heatmap},
    {"chargerTypeRatio", types},
    {"forecast24h", forecast},
    {"stations", stations()},
    {"recentEvents", db_.rows("SELECT action,detail,created_at AS createdAt "
                              "FROM audit_logs ORDER BY id DESC LIMIT 10")},
    {"forecastMeta",
     QJsonObject{{"generatedAt", forecastData_.value("generatedAt")},
                 {"modelVersion", forecastData_.value("modelVersion")},
                 {"source", forecastData_.value("source")}}}};
  return result;
}
