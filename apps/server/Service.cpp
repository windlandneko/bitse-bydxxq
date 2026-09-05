#include "Service.h"
#include <QCryptographicHash>
#include <QFile>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QUrlQuery>
#include <cmath>

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
  stopForecast();
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
    if (!qEnvironmentVariableIsSet("CHARGING_DISABLE_AUTO_FORECAST")) {
      QTimer::singleShot(1500, this, &Service::runForecast);
      auto *forecastTimer = new QTimer(this);
      connect(forecastTimer, &QTimer::timeout, this, &Service::runForecast);
      forecastTimer->start(15 * 60 * 1000);
    }
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
