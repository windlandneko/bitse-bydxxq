#include "Service.h"
#include <QUuid>
#include <algorithm>
#include <cmath>

QJsonObject Service::order(qint64 id) {
  auto result = db_.row(
    "SELECT o.id,o.order_no AS orderNo,o.user_id AS userId,o.station_id AS "
    "stationId,o.station_name AS stationName,o.charger_id AS "
    "chargerId,o.charger_code AS chargerCode,o.charger_type AS "
    "chargerType,o.power_kw AS powerKw,o.price_cents AS "
    "priceCents,o.status,o.created_at AS createdAt,o.expires_at AS "
    "expiresAt,o.started_at AS startedAt,o.ended_at AS "
    "endedAt,o.energy_wh/1000.0 AS energyKwh,o.duration_seconds AS "
    "durationSeconds,o.amount_cents AS amountCents,20.0+o.energy_wh/600.0 AS "
    "soc,o.stop_reason AS stopReason,COALESCE(w.balance_after,u.balance_cents) "
    "AS balanceCents FROM orders o JOIN users u ON u.id=o.user_id LEFT JOIN "
    "wallet_transactions w ON w.order_id=o.id WHERE o.id=?",
    {id});
  ensure(!result.isEmpty(), "NOT_FOUND", "订单不存在");
  return result;
}
QJsonArray Service::orders(const QJsonObject &p, const Principal &actor) {
  QString sql = "SELECT id FROM orders";
  QVariantList values;
  if (actor.role == "user") {
    sql += " WHERE user_id=?";
    values.append(actor.id);
  } else if (p.value("userId").toInt() > 0) {
    sql += " WHERE user_id=?";
    values.append(integer(p, "userId"));
  }
  sql += " ORDER BY created_at DESC,id DESC LIMIT 1000";
  QJsonArray result;
  for (auto row : db_.rows(sql, values))
    result.append(order(row.toObject().value("id").toInteger()));
  return result;
}
QJsonValue Service::orderAction(const QString &action, const QJsonObject &p,
                                const Principal &actor) {
  if (action == "orders.list" || action == "admin.orders")
    return orders(p, actor);
  if (action == "orders.active") {
    auto active = db_.row("SELECT id FROM orders WHERE user_id=? AND status IN "
                          "('reserved','charging','pending_payment')",
                          {actor.id});
    return active.isEmpty() ? QJsonValue(QJsonValue::Null)
                            : QJsonValue(order(active.value("id").toInteger()));
  }
  if (action == "orders.reserve") {
    requireActive(actor);
    auto cid = integer(p, "chargerId");
    auto active = db_.row("SELECT id FROM orders WHERE user_id=? AND status IN "
                          "('reserved','charging','pending_payment')",
                          {actor.id});
    ensure(active.isEmpty(), "ACTIVE_ORDER",
           "您有未完成的充电订单，请先结算或处理预约");
    ensure(user(actor.id).value("balanceCents").toInteger() > 0,
           "INSUFFICIENT_BALANCE", "余额不足，请先充值");
    auto charger = db_.row(
      "SELECT c.*,s.name AS station_name,s.price_cents FROM chargers c JOIN "
      "stations s ON s.id=c.station_id WHERE c.id=?",
      {cid});
    ensure(!charger.isEmpty(), "NOT_FOUND", "电桩不存在");
    ensure(charger.value("status").toString() == "idle", "CHARGER_BUSY",
           "该电桩刚被占用或暂不可用，请重新选择");
    auto now = QDateTime::currentDateTimeUtc();
    auto id = db_.insert(
      "INSERT INTO "
      "orders(order_no,user_id,station_id,charger_id,station_name,charger_code,"
      "charger_type,power_kw,price_cents,status,created_at,expires_at,time_"
      "scale) VALUES(?,?,?,?,?,?,?,?,?,'reserved',?,?,?)",
      {"CHG-" + now.toString("yyyyMMddHHmmss") + "-"
         + QUuid::createUuid().toString(QUuid::Id128).left(8).toUpper(),
       actor.id, charger.value("station_id").toInteger(), cid,
       charger.value("station_name").toString(),
       charger.value("code").toString(), charger.value("type").toString(),
       charger.value("power_kw").toDouble(),
       charger.value("price_cents").toInt(), now.toString(Qt::ISODateWithMs),
       now.addSecs(900).toString(Qt::ISODateWithMs), scale_});
    db_.execute("UPDATE chargers SET status='reserved' WHERE id=?", {cid});
    audit(actor, "预约电桩", charger.value("code").toString(),
          "预约保留15分钟");
    return order(id);
  }
  auto id = integer(p, "orderId");
  auto current = order(id);
  ensure(current.value("userId").toInteger() == actor.id, "FORBIDDEN",
         "您无权操作此订单");
  if (action == "orders.get") return current;
  auto status = current.value("status").toString();
  Transaction tx(db_);
  if (action == "orders.start") {
    if (status == "charging" || status == "pending_payment"
        || status == "paid") {
      tx.commit();
      return current;
    }
    requireActive(actor);
    ensure(status == "reserved", "ORDER_STATE", "仅预约订单可以开始充电");
    ensure(
      QDateTime::fromString(current.value("expiresAt").toString(), Qt::ISODate)
        > QDateTime::currentDateTimeUtc(),
      "RESERVATION_EXPIRED", "预约已过期，请重新选桩");
    auto budget = user(actor.id).value("balanceCents").toInteger();
    ensure(budget > 0, "INSUFFICIENT_BALANCE", "余额不足，请先充值");
    auto station = db_.row("SELECT price_cents FROM stations WHERE id=?",
                           {current.value("stationId").toInteger()});
    db_.execute(
      "UPDATE orders SET "
      "status='charging',started_at=?,budget_cents=?,price_cents=?,time_scale=?"
      " WHERE id=?",
      {utcNow(), budget, station.value("price_cents").toInt(), scale_, id});
    db_.execute("UPDATE chargers SET status='charging' WHERE id=?",
                {current.value("chargerId").toInteger()});
    audit(actor, "开始充电", current.value("chargerCode").toString(),
          "价格与余额预算已锁定");
  } else if (action == "orders.cancel") {
    ensure(status == "reserved" || status == "cancelled", "ORDER_STATE",
           "当前订单无法取消，请先结束充电");
    if (status == "reserved") {
      db_.execute(
        "UPDATE orders SET "
        "status='cancelled',ended_at=?,stop_reason='用户取消' WHERE id=?",
        {utcNow(), id});
      db_.execute(
        "UPDATE chargers SET status='idle' WHERE id=? AND status='reserved'",
        {current.value("chargerId").toInteger()});
      audit(actor, "取消预约", current.value("chargerCode").toString(),
            "电桩已释放");
    }
  } else if (action == "orders.stop") {
    ensure(status == "charging" || status == "pending_payment"
             || status == "paid",
           "ORDER_STATE", "订单尚未开始充电");
    if (status == "charging") finishOrder(id, "用户结束充电");
  } else if (action == "orders.settle") {
    ensure(status == "pending_payment" || status == "paid", "ORDER_STATE",
           "请先结束充电，再结算订单");
    if (status == "pending_payment") {
      auto amount = current.value("amountCents").toInteger();
      auto before = user(actor.id).value("balanceCents").toInteger();
      ensure(before >= amount, "INSUFFICIENT_BALANCE",
             "余额不足，请充值后结算");
      db_.execute("UPDATE users SET balance_cents=balance_cents-? WHERE id=?",
                  {amount, actor.id});
      db_.execute(
        "INSERT INTO "
        "wallet_transactions(user_id,order_id,kind,amount_cents,balance_before,"
        "balance_after,created_at) VALUES(?,?,'charge',?,?,?,?)",
        {actor.id, id, -amount, before, before - amount, utcNow()});
      db_.execute("UPDATE orders SET status='paid',settled_at=? WHERE id=?",
                  {utcNow(), id});
      audit(actor, "订单结算", current.value("orderNo").toString(),
            QString("实付 %1 元").arg(amount / 100.0, 0, 'f', 2));
    }
  } else
    throw BusinessError("UNKNOWN_ACTION", "不支持的订单操作");
  tx.commit();
  return order(id);
}
void Service::finishOrder(qint64 id, const QString &reason) {
  auto row = db_.row("SELECT * FROM orders WHERE id=? AND status='charging'",
                     {id});
  if (row.isEmpty()) return;
  const auto start = QDateTime::fromString(row.value("started_at").toString(),
                                           Qt::ISODate);
  const auto now = QDateTime::currentDateTimeUtc();
  const auto scale = row.value("time_scale").toInt();
  const double power = row.value("power_kw").toDouble();
  const auto price = row.value("price_cents").toInteger();
  const auto budget = row.value("budget_cents").toInteger();
  qint64 seconds = std::max<qint64>(0, start.msecsTo(now) * scale / 1000);
  qint64 cap = std::min<qint64>(48000, budget * 1000 / price);
  qint64 raw = std::llround(std::floor(power * 1000 * seconds / 3600.0));
  qint64 energy = std::min(raw, cap);
  QString why = reason;
  auto end = now;
  if (raw >= cap) {
    seconds = qint64(std::ceil(cap * 3600.0 / (power * 1000)));
    end = start.addMSecs(qint64(std::ceil(seconds * 1000.0 / scale)));
    why = cap < 48000 ? "达到起充余额预算，自动停止" : "电池已充满，自动停止";
  }
  qint64 amount = std::min(budget, (energy * price + 500) / 1000);
  db_.execute(
    "UPDATE orders SET "
    "status='pending_payment',ended_at=?,energy_wh=?,duration_seconds=?,amount_"
    "cents=?,stop_reason=? WHERE id=?",
    {end.toString(Qt::ISODateWithMs), energy, seconds, amount, why, id});
  auto cid = row.value("charger_id").toInteger();
  db_.execute("UPDATE chargers SET "
              "status='idle',charging_count=charging_count+1,charging_seconds="
              "charging_seconds+?,energy_wh=energy_wh+? WHERE id=?",
              {seconds, energy, cid});
  audit({"system", 0, {}}, "结束充电", row.value("charger_code").toString(),
        why);
}
void Service::tick() {
  const auto now = QDateTime::currentDateTimeUtc();
  for (auto it = tokens_.begin(); it != tokens_.end();) {
    if (it.value().expires <= now)
      it = tokens_.erase(it);
    else
      ++it;
  }
  Transaction tx(db_);
  for (auto value : db_.rows("SELECT id,charger_id,charger_code FROM orders "
                             "WHERE status='reserved' AND expires_at<=?",
                             {now.toString(Qt::ISODateWithMs)})) {
    auto r = value.toObject();
    db_.execute(
      "UPDATE orders SET status='cancelled',ended_at=?,stop_reason='预约超时' "
      "WHERE id=?",
      {utcNow(), r.value("id").toInteger()});
    db_.execute(
      "UPDATE chargers SET status='idle' WHERE id=? AND status='reserved'",
      {r.value("charger_id").toInteger()});
    audit({"system", 0, {}}, "预约超时", r.value("charger_code").toString(),
          "电桩已自动释放");
  }
  for (auto value : db_.rows("SELECT id,code FROM chargers WHERE "
                             "status='restarting' AND restart_at<=?",
                             {now.toString(Qt::ISODateWithMs)})) {
    auto r = value.toObject();
    db_.execute("UPDATE chargers SET status='idle',restart_at=NULL WHERE id=?",
                {r.value("id").toInteger()});
    audit({"system", 0, {}}, "设备重启完成", r.value("code").toString(),
          "模拟设备确认响应，恢复空闲");
  }
  for (auto value : db_.rows("SELECT * FROM orders WHERE status='charging'")) {
    auto r = value.toObject();
    auto start = QDateTime::fromString(r.value("started_at").toString(),
                                       Qt::ISODate);
    qint64 seconds = std::max<qint64>(
      0, start.msecsTo(now) * r.value("time_scale").toInt() / 1000);
    qint64 energy = std::llround(
      std::floor(r.value("power_kw").toDouble() * 1000 * seconds / 3600.0));
    auto price = r.value("price_cents").toInteger();
    auto cap = std::min<qint64>(48000, r.value("budget_cents").toInteger()
                                         * 1000 / price);
    if (energy >= cap)
      finishOrder(r.value("id").toInteger(), "自动停止");
    else
      db_.execute("UPDATE orders SET "
                  "energy_wh=?,duration_seconds=?,amount_cents=? WHERE id=?",
                  {energy, seconds, (energy * price + 500) / 1000,
                   r.value("id").toInteger()});
  }
  tx.commit();
}
