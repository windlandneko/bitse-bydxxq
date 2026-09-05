#include "Service.h"
#include <QUuid>
#include <algorithm>
#include <cmath>

QJsonArray Service::chargers(const QJsonObject &p) {
  QString sql = "SELECT c.id,c.station_id AS stationId,s.name AS "
                "stationName,c.code,c.type,c.power_kw AS "
                "powerKw,c.status,c.charging_count AS "
                "chargingCount,c.charging_seconds AS "
                "chargingSeconds,c.energy_wh/1000.0 AS energyKwh FROM chargers "
                "c JOIN stations s ON s.id=c.station_id WHERE 1=1";
  QVariantList values;
  if (p.value("stationId").toInt() > 0) {
    sql += " AND c.station_id=?";
    values.append(integer(p, "stationId"));
  }
  if (!p.value("status").toString().isEmpty()) {
    sql += " AND c.status=?";
    values.append(p.value("status").toString());
  }
  if (!p.value("query").toString().isEmpty()) {
    sql += " AND instr(c.code,?)>0";
    values.append(p.value("query").toString());
  }
  return db_.rows(sql + " ORDER BY c.id", values);
}
QJsonArray Service::stations(const QJsonObject &p) {
  double lat = p.value("latitude").toDouble(31.230416),
         lon = p.value("longitude").toDouble(121.473701);
  ensure(std::isfinite(lat) && std::isfinite(lon) && lat >= -90 && lat <= 90
           && lon >= -180 && lon <= 180,
         "VALIDATION_ERROR", "经纬度超出范围");
  auto rows = db_.rows(
    "SELECT "
    "s.id,s.code,s.name,s.address,s.region,s.latitude,s.longitude,s.price_"
    "cents AS priceCents,COUNT(c.id) AS totalChargers,SUM(CASE WHEN "
    "c.status='idle' THEN 1 ELSE 0 END) AS idleChargers,100.0*SUM(CASE WHEN "
    "c.status IN ('idle','reserved','charging') THEN 1 ELSE 0 "
    "END)/MAX(COUNT(c.id),1) AS onlineRate FROM stations s LEFT JOIN chargers "
    "c ON c.station_id=s.id GROUP BY s.id ORDER BY s.id");
  const auto predicted = forecastData_.value("stations").toArray();
  const bool fresh = QDateTime::fromString(
                       forecastData_.value("generatedAt").toString(),
                       Qt::ISODate)
                       .secsTo(QDateTime::currentDateTimeUtc())
                   < 7200;
  QList<QJsonObject> items;
  for (auto value : rows) {
    auto s = value.toObject();
    auto query = p.value("query").toString().trimmed();
    if (!query.isEmpty()
        && !s.value("name").toString().contains(query, Qt::CaseInsensitive)
        && !s.value("address").toString().contains(query, Qt::CaseInsensitive))
      continue;
    auto region = p.value("region").toString();
    if (!region.isEmpty() && s.value("region").toString() != region) continue;
    if (p.value("fastOnly").toBool()
        && db_
             .row("SELECT id FROM chargers WHERE station_id=? AND type='dc' "
                  "LIMIT 1",
                  {s.value("id").toInt()})
             .isEmpty())
      continue;
    constexpr double rad = 3.14159265358979323846 / 180;
    double dlat = (s.value("latitude").toDouble() - lat) * rad,
           dlon = (s.value("longitude").toDouble() - lon) * rad;
    double a = std::pow(std::sin(dlat / 2), 2)
             + std::cos(lat * rad)
                 * std::cos(s.value("latitude").toDouble() * rad)
                 * std::pow(std::sin(dlon / 2), 2);
    s.insert("distanceKm",
             6371 * 2 * std::asin(std::sqrt(std::clamp(a, 0.0, 1.0))));
    int free = -1;
    if (fresh)
      for (auto f : predicted) {
        auto obj = f.toObject();
        if (obj.value("stationId") == s.value("id")) {
          auto hours = obj.value("hours").toArray();
          if (!hours.isEmpty())
            free = hours.first()
                     .toObject()
                     .value("availableChargers")
                     .toInt(-1);
        }
      }
    s.insert("predictedAvailableChargers", free);
    s.insert("recommended",
             free >= std::max(1, s.value("totalChargers").toInt() / 2)
               && s.value("idleChargers").toInt() > 0);
    s.insert("forecastAt", free < 0
                             ? QString()
                             : forecastData_.value("generatedAt").toString());
    items.append(s);
  }
  auto sort = p.value("sort").toString("distance");
  std::stable_sort(items.begin(), items.end(),
                   [sort](const QJsonObject &a, const QJsonObject &b) {
                     if (sort == "price")
                       return a.value("priceCents").toInt()
                            < b.value("priceCents").toInt();
                     if (sort == "idle")
                       return a.value("idleChargers").toInt()
                            > b.value("idleChargers").toInt();
                     return a.value("distanceKm").toDouble()
                          < b.value("distanceKm").toDouble();
                   });
  QJsonArray result;
  for (auto item : items) result.append(item);
  return result;
}
QJsonValue Service::stationAction(const QString &action, const QJsonObject &p,
                                  const Principal &actor) {
  if (action == "stations.list" || action == "admin.stations")
    return stations(p);
  if (action == "admin.chargers") return chargers(p);
  if (action == "location.presets")
    return QJsonArray{QJsonObject{{"name", "人民广场"},
                                  {"latitude", 31.230416},
                                  {"longitude", 121.473701}},
                      QJsonObject{{"name", "陆家嘴"},
                                  {"latitude", 31.235929},
                                  {"longitude", 121.501116}},
                      QJsonObject{{"name", "中山公园"},
                                  {"latitude", 31.220000},
                                  {"longitude", 121.420000}},
                      QJsonObject{{"name", "上海南站"},
                                  {"latitude", 31.154425},
                                  {"longitude", 121.429252}},
                      QJsonObject{{"name", "静安北城"},
                                  {"latitude", 31.280000},
                                  {"longitude", 121.453000}}};
  if (action == "stations.detail") {
    auto id = integer(p, "stationId");
    for (auto value : stations(p))
      if (value.toObject().value("id").toInteger() == id)
        return QJsonObject{{"station", value},
                           {"chargers", chargers({{"stationId", id}})}};
    throw BusinessError("NOT_FOUND", "电站不存在");
  }
  if (action == "admin.station.save") {
    auto name = requiredText(p, "name", 60),
         address = requiredText(p, "address", 200);
    auto region = p.value("region").toString("").trimmed();
    ensure(region.size() <= 60, "VALIDATION_ERROR", "区域名称过长");
    double lat = p.value("latitude").toDouble(999),
           lon = p.value("longitude").toDouble(999);
    ensure(p.value("latitude").isDouble() && p.value("longitude").isDouble()
             && lat >= -90 && lat <= 90 && lon >= -180 && lon <= 180,
           "VALIDATION_ERROR", "请填写有效经纬度");
    auto price = integer(p, "priceCents", 1, 10000);
    qint64 id = p.value("id").toInteger();
    Transaction tx(db_);
    if (id > 0) {
      ensure(!db_.row("SELECT id FROM stations WHERE id=?", {id}).isEmpty(),
             "NOT_FOUND", "电站不存在");
      db_.execute("UPDATE stations SET "
                  "name=?,address=?,region=?,latitude=?,longitude=?,price_"
                  "cents=? WHERE id=?",
                  {name, address, region, lat, lon, price, id});
    } else {
      auto count = integer(p, "chargerCount", 1, 100);
      auto type = p.value("type").toString();
      ensure(type.isEmpty() || type == "ac" || type == "dc", "VALIDATION_ERROR",
             "设备类型无效");
      auto power = p.value("powerKw").toDouble(type == "ac" ? 7 : 60);
      ensure(power > 0 && power <= 1000, "VALIDATION_ERROR",
             "功率须在0至1000kW范围内");
      auto code = "ST-"
                + QUuid::createUuid().toString(QUuid::Id128).left(8).toUpper();
      id = db_.insert("INSERT INTO "
                      "stations(code,name,address,region,latitude,longitude,"
                      "price_cents,created_at) VALUES(?,?,?,?,?,?,?,?)",
                      {code, name, address, region, lat, lon, price, utcNow()});
      for (int c = 1; c <= count; ++c) {
        auto chargerType = type.isEmpty()
                           ? (c % 2 ? QString("dc") : QString("ac"))
                           : type;
        auto kw = type.isEmpty() ? (chargerType == "ac" ? 7.0 : power) : power;
        db_.execute(
          "INSERT INTO chargers(station_id,code,type,power_kw,created_at) "
          "VALUES(?,?,?,?,?)",
          {id, code + QString("-%1").arg(c, 3, 10, QChar('0')), chargerType, kw,
           utcNow()});
      }
    }
    audit(actor, "保存电站", QString::number(id), name);
    tx.commit();
    for (auto value : stations())
      if (value.toObject().value("id").toInteger() == id) return value;
  }
  if (action == "admin.charger.restart" || action == "admin.charger.status") {
    auto id = integer(p, "chargerId");
    auto charger = db_.row("SELECT status,code FROM chargers WHERE id=?", {id});
    ensure(!charger.isEmpty(), "NOT_FOUND", "电桩不存在");
    auto current = charger.value("status").toString();
    ensure(current != "charging" && current != "reserved"
             && current != "restarting",
           "CHARGER_BUSY", "电桩正在使用或重启中，请先处理当前任务");
    Transaction tx(db_);
    if (action == "admin.charger.restart") {
      db_.execute(
        "UPDATE chargers SET status='restarting',restart_at=? WHERE id=?",
        {QDateTime::currentDateTimeUtc().addSecs(2).toString(Qt::ISODateWithMs),
         id});
      audit(actor, "下发远程重启", charger.value("code").toString(),
            "模拟控制指令已入队，等待设备恢复");
      tx.commit();
      return QJsonObject{{"message", "重启指令已发送，约2秒后更新设备状态"}};
    }
    auto status = requiredText(p, "status", 10);
    ensure(status == "idle" || status == "fault", "VALIDATION_ERROR",
           "设备状态无效");
    db_.execute("UPDATE chargers SET status=? WHERE id=?", {status, id});
    audit(actor, status == "fault" ? "标记设备故障" : "恢复设备",
          charger.value("code").toString(), "管理员操作");
    tx.commit();
    return QJsonObject{};
  }
  throw BusinessError("UNKNOWN_ACTION", "不支持的电站操作");
}
