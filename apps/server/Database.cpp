#include "Database.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QPasswordDigestor>
#include <QRandomGenerator>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTimeZone>
#include <QUuid>
#include <cmath>

QString utcNow() {
  return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}
QString passwordHash(const QString &password, const QString &salt) {
  return QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Sha256,
                                            password.toUtf8(), salt.toUtf8(),
                                            100000, 32)
    .toHex();
}
QJsonObject success(const QJsonValue &data) {
  return {{"ok", true}, {"data", data}};
}
QJsonObject failure(const QString &code, const QString &message) {
  return {{"ok", false},
          {"error", QJsonObject{{"code", code}, {"message", message}}}};
}

namespace {
QSqlQuery query(const QSqlDatabase &db, const QString &sql,
                const QVariantList &values) {
  QSqlQuery q(db);
  if (!q.prepare(sql))
    throw BusinessError("DATABASE_ERROR",
                        "数据库查询准备失败：" + q.lastError().text());
  for (const auto &value : values) q.addBindValue(value);
  if (!q.exec())
    throw BusinessError("DATABASE_ERROR",
                        "数据库操作失败：" + q.lastError().text());
  return q;
}

QJsonObject currentRow(const QSqlQuery &query, const QSqlRecord &record) {
  QJsonObject result;
  for (int column = 0; column < record.count(); ++column)
    result.insert(record.fieldName(column),
                  QJsonValue::fromVariant(query.value(column)));
  return result;
}
} // namespace

void Database::execute(const QString &sql, const QVariantList &values) {
  query(db_, sql, values);
}
qint64 Database::insert(const QString &sql, const QVariantList &values) {
  return query(db_, sql, values).lastInsertId().toLongLong();
}
QJsonArray Database::rows(const QString &sql, const QVariantList &values) {
  auto q = query(db_, sql, values);
  const auto record = q.record();
  QJsonArray result;
  while (q.next()) result.append(currentRow(q, record));
  return result;
}
QJsonObject Database::row(const QString &sql, const QVariantList &values) {
  auto q = query(db_, sql, values);
  return q.next() ? currentRow(q, q.record()) : QJsonObject{};
}
void Database::begin() { execute("BEGIN IMMEDIATE"); }
void Database::commit() {
  if (!db_.commit()) throw BusinessError("DATABASE_ERROR", "数据库提交失败");
}
void Database::rollback() { db_.rollback(); }
void Database::open(const QString &directory, bool shouldSeed) {
  directory_ = QDir(directory).absolutePath();
  if (!QDir().mkpath(directory_ + "/uploads"))
    throw BusinessError("DATABASE_ERROR", "无法创建数据目录");
  db_ = QSqlDatabase::addDatabase("QSQLITE", "service");
  db_.setDatabaseName(directory_ + "/platform.db");
  if (!db_.open())
    throw BusinessError("DATABASE_ERROR",
                        "无法打开数据库：" + db_.lastError().text());
  auto version = row("PRAGMA user_version").value("user_version").toInt();
  if (version != 0 && version != 2)
    throw BusinessError("SCHEMA_VERSION",
                        "数据库版本不受支持，请使用新的数据目录");
  if (version == 0
      && !row("SELECT name FROM sqlite_master WHERE type='table'").isEmpty())
    throw BusinessError("SCHEMA_VERSION",
                        "此目录包含旧版数据库，请保留原文件并指定新的数据目录");
  QFile schema(":/database/schema.sql");
  if (!schema.open(QIODevice::ReadOnly))
    throw BusinessError("DATABASE_ERROR", "缺少内置数据库定义");
  execute("PRAGMA foreign_keys = ON");
  execute("PRAGMA journal_mode = WAL");
  execute("PRAGMA busy_timeout = 5000");
  Transaction initialization(*this);
  for (const auto &sql : QString::fromUtf8(schema.readAll()).split(';')) {
    auto statement = sql.trimmed();
    if (statement.isEmpty()) continue;
    if (statement.startsWith("PRAGMA")
        && !statement.startsWith("PRAGMA user_version"))
      continue;
    execute(statement);
  }
  if (row("PRAGMA quick_check").value("quick_check").toString() != "ok")
    throw BusinessError("DATABASE_ERROR", "数据库完整性检查失败，请恢复备份");
  if (row("SELECT id FROM admins LIMIT 1").isEmpty()) {
    auto salt = QUuid::createUuid().toString(QUuid::WithoutBraces);
    execute("INSERT INTO admins(username,salt,password_hash) VALUES(?,?,?)",
            {"admin", salt, passwordHash("123456", salt)});
  }
  if (shouldSeed && version == 0) seed();
  initialization.commit();
}
void Database::seed() {
  const QString now = utcNow();
  auto user1 = insert(
    "INSERT INTO users(phone,nickname,balance_cents,created_at) "
    "VALUES(?,?,?,?)",
    {"13800000001", "小林", 5000000, now});
  auto user2 = insert(
    "INSERT INTO users(phone,nickname,balance_cents,created_at) "
    "VALUES(?,?,?,?)",
    {"13800000002", "小周", 5000000, now});
  for (auto uid : {user1, user2})
    execute(
      "INSERT INTO "
      "wallet_transactions(user_id,kind,amount_cents,balance_before,balance_"
      "after,created_at) VALUES(?,'recharge',5000000,0,5000000,?)",
      {uid, QDateTime::currentDateTimeUtc().addDays(-40).toString(
              Qt::ISODateWithMs)});
  struct StationSeed {
    const char *name;
    const char *address;
    const char *region;
    double lat, lon;
    int price;
  };
  const StationSeed seeds[] = {{"人民广场充电站", "上海市黄浦区人民大道200号",
                                "黄浦区", 31.230416, 121.473701, 118},
                               {"陆家嘴滨江充电站", "上海市浦东新区富城路99号",
                                "浦东新区", 31.235929, 121.501116, 135},
                               {"中山公园充电站", "上海市长宁区长宁路780号",
                                "长宁区", 31.220000, 121.420000, 108},
                               {"徐汇南站充电站", "上海市徐汇区沪闵路9001号",
                                "徐汇区", 31.154425, 121.429252, 99},
                               {"静安北城充电站", "上海市静安区共和新路2188号",
                                "静安区", 31.280000, 121.453000, 125}};
  QRandomGenerator rng(20260905);
  int index = 0;
  for (const auto &s : seeds) {
    ++index;
    auto sid = insert("INSERT INTO "
                      "stations(code,name,address,region,latitude,longitude,"
                      "price_cents,created_at) VALUES(?,?,?,?,?,?,?,?)",
                      {QString("ST%1").arg(index, 3, 10, QChar('0')),
                       QString::fromUtf8(s.name), QString::fromUtf8(s.address),
                       QString::fromUtf8(s.region), s.lat, s.lon, s.price,
                       now});
    for (int c = 1; c <= 6; ++c) {
      bool dc = c > 3;
      QString code = QString("ST%1-%2%3")
                       .arg(index, 3, 10, QChar('0'))
                       .arg(dc ? "DC" : "AC")
                       .arg(c, 2, 10, QChar('0'));
      auto cid = insert(
        "INSERT INTO chargers(station_id,code,type,power_kw,status,created_at) "
        "VALUES(?,?,?,?,?,?)",
        {sid, code, dc ? "dc" : "ac", dc ? 60 : 7, c == 3 ? "fault" : "idle",
         now});
      if (c == 3) continue;
      for (int day = 35; day >= 0; --day) {
        int hour = (dc ? 8 : 18) + int(rng.bounded(4u));
        auto start = QDateTime(QDate::currentDate().addDays(-day),
                               QTime(hour, int(rng.bounded(60u))),
                               QTimeZone("Asia/Shanghai"))
                       .toUTC();
        int seconds = dc ? 1200 + int(rng.bounded(1800u))
                         : 1800 + int(rng.bounded(5400u));
        auto end = start.addSecs(seconds);
        if (end > QDateTime::currentDateTimeUtc()) continue;
        qint64 energy = std::llround((dc ? 60.0 : 7.0) * 1000 * seconds / 3600);
        int amount = int((energy * s.price + 500) / 1000);
        auto uid = ((c + day) % 2) ? user1 : user2;
        auto oid = insert(
          "INSERT INTO "
          "orders(order_no,user_id,station_id,charger_id,station_name,charger_"
          "code,charger_type,power_kw,price_cents,status,created_at,expires_at,"
          "started_at,ended_at,energy_wh,duration_seconds,amount_cents,settled_"
          "at,time_scale) VALUES(?,?,?,?,?,?,?,?,?,'paid',?,?,?,?,?,?,?,?,1)",
          {QString("DEMO-%1-%2-%3").arg(sid).arg(cid).arg(day), uid, sid, cid,
           QString::fromUtf8(s.name), code, dc ? "dc" : "ac", dc ? 60 : 7,
           s.price, start.toString(Qt::ISODateWithMs),
           start.addSecs(900).toString(Qt::ISODateWithMs),
           start.toString(Qt::ISODateWithMs), end.toString(Qt::ISODateWithMs),
           energy, seconds, amount, end.toString(Qt::ISODateWithMs)});
        auto before = row("SELECT balance_cents FROM users WHERE id=?", {uid})
                        .value("balance_cents")
                        .toInt();
        execute("UPDATE users SET balance_cents=balance_cents-? WHERE id=?",
                {amount, uid});
        execute(
          "INSERT INTO "
          "wallet_transactions(user_id,order_id,kind,amount_cents,balance_"
          "before,balance_after,created_at) VALUES(?,?,'charge',?,?,?,?)",
          {uid, oid, -amount, before, before - amount,
           end.toString(Qt::ISODateWithMs)});
        execute("UPDATE chargers SET "
                "charging_count=charging_count+1,charging_seconds=charging_"
                "seconds+?,energy_wh=energy_wh+? WHERE id=?",
                {seconds, energy, cid});
      }
    }
  }
  execute("INSERT INTO audit_logs(actor,action,target,detail,created_at) "
          "VALUES('system','演示环境初始化','平台','"
          "5个上海模拟站点，含35天可复现演示订单；钱包流水按数据库顺序对账',?)",
          {now});
}
