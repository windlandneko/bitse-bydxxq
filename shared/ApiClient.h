#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QObject>
#include <QVariantMap>
#include <functional>

class ApiClient final : public QObject {
  Q_OBJECT
  Q_PROPERTY(
    QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)
  Q_PROPERTY(QString token READ token WRITE setToken NOTIFY tokenChanged)
public:
  explicit ApiClient(QObject *parent = nullptr);
  QString baseUrl() const { return baseUrl_; }
  QString token() const { return token_; }
  void setBaseUrl(const QString &url);
  void setToken(const QString &token);
  Q_INVOKABLE void request(QString action, QVariantMap params = {},
                           QString tag = {});
  void call(const QString &action, const QJsonObject &params,
            std::function<void(QJsonValue)> success,
            std::function<void(QString)> failure = {});
signals:
  void baseUrlChanged();
  void tokenChanged();
  void succeeded(QString tag, QVariant data);
  void failed(QString tag, QString message, QString code);

private:
  void send(const QString &action, const QJsonObject &params,
            std::function<void(QJsonValue)> success,
            std::function<void(QString, QString)> failure);
  QNetworkAccessManager network_;
  QString baseUrl_;
  QString token_;
};
