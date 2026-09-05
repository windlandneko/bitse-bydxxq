#pragma once
#include "Database.h"
#include <QDateTime>
#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QProcess>
#include <QTimer>

struct Principal {
  QString role;
  qint64 id = 0;
  QDateTime expires;
};

class Service final : public QObject {
  Q_OBJECT
public:
  Service(QString directory, QString sourceRoot, int scale, bool seed);
public slots:
  void initialize();
  void shutdown();
  void request(quint64 id, QJsonObject input, QString token);
signals:
  void ready();
  void fatal(QString message);
  void replied(quint64 id, QJsonObject result);

private:
  QJsonValue dispatch(const QString &action, const QJsonObject &p,
                      const Principal &actor);
  Principal authorize(const QString &action, const QString &token);
  QJsonObject user(qint64 id);
  QJsonArray stations(const QJsonObject &params = {});
  QJsonArray chargers(const QJsonObject &params = {});
  QJsonObject order(qint64 id);
  QJsonArray orders(const QJsonObject &params, const Principal &actor);
  QJsonValue accountAction(const QString &action, const QJsonObject &p,
                           const Principal &actor);
  QJsonValue stationAction(const QString &action, const QJsonObject &p,
                           const Principal &actor);
  QJsonValue orderAction(const QString &action, const QJsonObject &p,
                         const Principal &actor);
  QJsonObject overview(int days);
  QJsonObject dashboard();
  void tick();
  void finishOrder(qint64 id, const QString &reason);
  void geocode(quint64 requestId, const QJsonObject &params);
  void runForecast();
  QJsonObject forecasts() const;
  void audit(const Principal &actor, const QString &action,
             const QString &target, const QString &detail);
  void requireActive(const Principal &actor);
  Database db_;
  QString directory_, sourceRoot_;
  int scale_;
  bool seed_;
  QHash<QString, Principal> tokens_;
  QHash<QString, QPair<int, QDateTime>> loginFailures_;
  QTimer *timer_ = nullptr;
  QNetworkAccessManager *network_ = nullptr;
  QProcess *forecastProcess_ = nullptr;
  QJsonObject forecastData_;
  QString forecastError_, forecastRunAt_;
};

QString requiredText(const QJsonObject &p, const QString &key,
                     int maxLength = 100);
qint64 integer(const QJsonObject &p, const QString &key, qint64 minimum = 1,
               qint64 maximum = 2147483647);
void ensure(bool condition, const QString &code, const QString &message);
