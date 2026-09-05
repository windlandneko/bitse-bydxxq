#include "ApiClient.h"

#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

ApiClient::ApiClient(QObject *parent) : QObject(parent) {
  setBaseUrl(
    qEnvironmentVariable("CHARGING_SERVER_URL", "http://127.0.0.1:8080"));
}

void ApiClient::setBaseUrl(const QString &url) {
  QString value = url.trimmed();
  while (value.endsWith('/')) value.chop(1);
  if (baseUrl_ == value) return;
  baseUrl_ = value;
  emit baseUrlChanged();
}

void ApiClient::setToken(const QString &token) {
  if (token_ == token) return;
  token_ = token;
  emit tokenChanged();
}

void ApiClient::call(const QString &action, const QJsonObject &params,
                     std::function<void(QJsonValue)> success,
                     std::function<void(QString)> failure) {
  send(action, params, std::move(success),
       [this, action, failure](QString message, QString code) {
         if (failure)
           failure(message);
         else
           emit failed(action, message, code);
       });
}

void ApiClient::send(const QString &action, const QJsonObject &params,
                     std::function<void(QJsonValue)> success,
                     std::function<void(QString, QString)> failure) {
  QNetworkRequest request{QUrl(baseUrl_ + "/api/rpc")};
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  request.setRawHeader("Connection", "close");
  request.setTransferTimeout(15000);
  if (!token_.isEmpty())
    request.setRawHeader("Authorization", "Bearer " + token_.toUtf8());
  auto *reply = network_.post(
    request, QJsonDocument(QJsonObject{{"action", action}, {"params", params}})
               .toJson(QJsonDocument::Compact));
  const QString sentToken = token_;
  connect(
    reply, &QNetworkReply::finished, this,
    [this, reply, action, sentToken, success, failure] {
      const auto error = reply->error();
      const auto bytes = reply->readAll();
      reply->deleteLater();
      QJsonParseError parseError;
      const auto doc = QJsonDocument::fromJson(bytes, &parseError);
      if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        failure(error == QNetworkReply::NoError ? "服务返回了无效数据"
                                                : "无法连接服务，请稍后重试",
                "NETWORK_ERROR");
        return;
      }
      const auto result = doc.object();
      if (!result.value("ok").toBool()) {
        const auto err = result.value("error").toObject();
        failure(err.value("message").toString("操作失败，请重试"),
                err.value("code").toString("SERVER_ERROR"));
        return;
      }
      const auto data = result.value("data");
      if (action == "user.login" || action == "admin.login")
        setToken(data.toObject().value("token").toString());
      if (action == "auth.logout" && token_ == sentToken) setToken({});
      if (success) success(data);
    });
}
