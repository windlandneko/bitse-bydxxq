#include "Service.h"
#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QRegularExpression>
#include <QSaveFile>
#include <QUuid>

QJsonObject Service::user(qint64 id) {
  auto result = db_.row(
    "SELECT id,phone,nickname,avatar_url AS avatarUrl,balance_cents AS "
    "balanceCents,status,created_at AS createdAt FROM users WHERE id=?",
    {id});
  ensure(!result.isEmpty(), "NOT_FOUND", "用户不存在");
  return result;
}
QJsonValue Service::accountAction(const QString &action, const QJsonObject &p,
                                  const Principal &actor) {
  if (action == "user.login") {
    QString phone = requiredText(p, "phone", 11);
    ensure(QRegularExpression("^1[0-9]{10}$").match(phone).hasMatch(),
           "VALIDATION_ERROR", "请输入有效的11位手机号");
    auto found = db_.row("SELECT id FROM users WHERE phone=?", {phone});
    qint64 id = found.value("id").toInteger();
    if (found.isEmpty())
      id = db_.insert(
        "INSERT INTO users(phone,nickname,created_at) VALUES(?,?,?)",
        {phone, "用户" + phone.right(4), utcNow()});
    auto profile = user(id);
    ensure(profile.value("status").toString() == "active", "ACCOUNT_FROZEN",
           "账号已冻结，请联系运营人员");
    QString token = QUuid::createUuid().toString(QUuid::WithoutBraces)
                  + QUuid::createUuid().toString(QUuid::WithoutBraces);
    tokens_.insert(
      token, {"user", id, QDateTime::currentDateTimeUtc().addSecs(24 * 3600)});
    return QJsonObject{{"token", token}, {"user", profile}};
  }
  if (action == "admin.login") {
    auto username = requiredText(p, "username", 60),
         password = requiredText(p, "password", 128);
    auto previous = loginFailures_.value(username);
    ensure(previous.second <= QDateTime::currentDateTimeUtc(), "LOGIN_LOCKED",
           "尝试次数过多，请30秒后重试");
    auto account = db_.row(
      "SELECT id,salt,password_hash FROM admins WHERE username=?", {username});
    auto supplied = passwordHash(
      password, account.value("salt").toString("invalid-account"));
    auto stored = account.value("password_hash").toString();
    unsigned int difference = supplied.size() ^ stored.size();
    for (int i = 0; i < qMin(supplied.size(), stored.size()); ++i)
      difference |= supplied[i].unicode() ^ stored[i].unicode();
    if (account.isEmpty() || difference != 0) {
      previous.first++;
      if (previous.first >= 5) {
        previous.second = QDateTime::currentDateTimeUtc().addSecs(30);
        previous.first = 0;
      }
      loginFailures_.insert(username, previous);
      throw BusinessError("LOGIN_FAILED", "账号或密码错误");
    }
    loginFailures_.remove(username);
    auto token = QUuid::createUuid().toString(QUuid::WithoutBraces)
               + QUuid::createUuid().toString(QUuid::WithoutBraces);
    tokens_.insert(token, {"admin", account.value("id").toInteger(),
                           QDateTime::currentDateTimeUtc().addSecs(12 * 3600)});
    return QJsonObject{{"token", token}, {"username", username}};
  }
  if (action == "user.me") return user(actor.id);
  if (action == "user.update") {
    auto profile = user(actor.id);
    auto nickname = p.contains("nickname")
                    ? requiredText(p, "nickname", 20)
                    : profile.value("nickname").toString();
    QString avatar = profile.value("avatarUrl").toString();
    if (p.contains("avatarBase64")) {
      auto encoded = requiredText(p, "avatarBase64", 2800000);
      auto bytes = QByteArray::fromBase64(encoded.toLatin1());
      ensure(!bytes.isEmpty() && bytes.size() <= 2 * 1024 * 1024,
             "INVALID_IMAGE", "头像请使用2MB以内的PNG或JPEG图片");
      QImageReader::setAllocationLimit(16);
      QBuffer buffer(&bytes);
      buffer.open(QIODevice::ReadOnly);
      QImageReader reader(&buffer);
      auto format = reader.format().toLower();
      ensure(format == "png" || format == "jpeg" || format == "jpg",
             "INVALID_IMAGE", "仅支持PNG和JPEG头像");
      auto size = reader.size();
      ensure(size.width() > 0 && size.height() > 0
               && qint64(size.width()) * size.height() <= 4000000,
             "INVALID_IMAGE", "头像尺寸过大，请选择不超过400万像素的图片");
      auto image = reader.read();
      ensure(!image.isNull(), "INVALID_IMAGE", "无法读取图片，请重新选择");
      avatar = "/uploads/" + QUuid::createUuid().toString(QUuid::WithoutBraces)
             + ".png";
      QSaveFile file(directory_ + avatar);
      ensure(
        file.open(QIODevice::WriteOnly)
          && image
               .scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation)
               .save(&file, "PNG")
          && file.commit(),
        "IO_ERROR", "头像保存失败");
    }
    db_.execute("UPDATE users SET nickname=?,avatar_url=? WHERE id=?",
                {nickname, avatar, actor.id});
    return user(actor.id);
  }
  if (action == "wallet.recharge") {
    requireActive(actor);
    auto amount = integer(p, "amountCents", 1, 1000000);
    auto before = user(actor.id).value("balanceCents").toInteger();
    ensure(before + amount <= 100000000, "BALANCE_LIMIT",
           "钱包余额已达到演示上限");
    db_.execute("UPDATE users SET balance_cents=balance_cents+? WHERE id=?",
                {amount, actor.id});
    db_.execute("INSERT INTO "
                "wallet_transactions(user_id,kind,amount_cents,balance_before,"
                "balance_after,created_at) VALUES(?,'recharge',?,?,?,?)",
                {actor.id, amount, before, before + amount, utcNow()});
    audit(actor, "钱包充值", QString::number(actor.id),
          QString("模拟充值 %1 元").arg(amount / 100.0, 0, 'f', 2));
    return user(actor.id);
  }
  if (action == "admin.users") {
    auto query = p.value("query").toString("").trimmed();
    ensure(query.size() <= 50, "VALIDATION_ERROR", "搜索内容过长");
    return db_.rows(
      "SELECT id,phone,nickname,avatar_url AS avatarUrl,balance_cents AS "
      "balanceCents,status,created_at AS createdAt FROM users WHERE "
      "instr(phone,?)>0 ORDER BY id DESC",
      {query});
  }
  if (action == "admin.user.status") {
    auto id = integer(p, "userId");
    auto status = requiredText(p, "status", 10);
    ensure(status == "active" || status == "frozen", "VALIDATION_ERROR",
           "用户状态无效");
    user(id);
    Transaction tx(db_);
    db_.execute("UPDATE users SET status=? WHERE id=?", {status, id});
    audit(actor, status == "frozen" ? "冻结用户" : "解冻用户",
          QString::number(id), "已有订单保留，可停止及结算");
    tx.commit();
    return user(id);
  }
  throw BusinessError("UNKNOWN_ACTION", "不支持的操作");
}
