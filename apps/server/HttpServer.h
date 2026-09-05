#pragma once
#include <QHash>
#include <QJsonObject>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>

class HttpServer final : public QTcpServer {
  Q_OBJECT
public:
  HttpServer(QString webRoot, QString dataRoot, QObject *parent = nullptr);
public slots:
  void respond(quint64 id, QJsonObject result);
signals:
  void request(quint64 id, QJsonObject input, QString token);

private:
  struct Pending {
    QPointer<QTcpSocket> socket;
    bool raw = false;
  };
  void acceptConnections();
  void process(QTcpSocket *socket);
  void reply(QTcpSocket *socket, int status, const QByteArray &body,
             const QByteArray &type = "application/json; charset=utf-8");
  void staticFile(QTcpSocket *socket, const QString &path);
  QString webRoot_, dataRoot_;
  QHash<quint64, Pending> pending_;
  quint64 nextId_ = 0;
  int activeConnections_ = 0;
};
