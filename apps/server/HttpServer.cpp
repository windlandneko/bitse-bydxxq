#include "HttpServer.h"
#include "Database.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QMimeDatabase>
#include <QTimer>
#include <QUrl>

HttpServer::HttpServer(QString webRoot, QString dataRoot, QObject *parent)
    : QTcpServer(parent), webRoot_(QDir(webRoot).absolutePath()),
      dataRoot_(QDir(dataRoot).absolutePath()) {
  connect(this, &QTcpServer::newConnection, this,
          &HttpServer::acceptConnections);
  setMaxPendingConnections(64);
}
void HttpServer::acceptConnections() {
  while (hasPendingConnections()) {
    auto *socket = nextPendingConnection();
    if (activeConnections_ >= 64) {
      connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
      reply(socket, 503,
            QJsonDocument(failure("SERVER_BUSY", "连接数已满，请稍后重试"))
              .toJson(QJsonDocument::Compact));
      continue;
    }
    ++activeConnections_;
    socket->setReadBufferSize(3 * 1024 * 1024 + 16384);
    auto *timeout = new QTimer(socket);
    timeout->setSingleShot(true);
    timeout->start(20000);
    connect(timeout, &QTimer::timeout, socket, [this, socket] {
      reply(socket, 408,
            QJsonDocument(failure("TIMEOUT", "请求超时"))
              .toJson(QJsonDocument::Compact));
    });
    connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
      process(socket);
    });
    connect(socket, &QTcpSocket::disconnected, this, [this, socket] {
      --activeConnections_;
      for (auto it = pending_.begin(); it != pending_.end();) {
        if (it.value().socket == socket)
          it = pending_.erase(it);
        else
          ++it;
      }
      socket->deleteLater();
    });
  }
}
void HttpServer::process(QTcpSocket *socket) {
  if (socket->property("dispatched").toBool()) return;
  auto bytes = socket->property("buffer").toByteArray() + socket->readAll();
  auto bad = [this, socket](int status, QString message) {
    reply(socket, status,
          QJsonDocument(failure("BAD_REQUEST", message))
            .toJson(QJsonDocument::Compact));
  };
  if (bytes.size() > 3 * 1024 * 1024 + 16384) {
    bad(413, "请求内容过大");
    return;
  }
  auto delimiter = bytes.indexOf("\r\n\r\n");
  if (delimiter < 0) {
    if (bytes.size() > 16384)
      bad(431, "请求头过大");
    else
      socket->setProperty("buffer", bytes);
    return;
  }
  if (delimiter > 16384) {
    bad(431, "请求头过大");
    return;
  }
  auto lines = bytes.left(delimiter).split('\n');
  auto first = lines.takeFirst().trimmed().split(' ');
  if (first.size() != 3 || first[2] != "HTTP/1.1") {
    bad(400, "仅支持HTTP/1.1请求");
    return;
  }
  auto method = first[0];
  if (method != "GET" && method != "POST" && method != "OPTIONS") {
    bad(405, "请求方法不支持");
    return;
  }
  QHash<QByteArray, QByteArray> headers;
  for (auto line : lines) {
    auto pos = line.indexOf(':');
    if (pos <= 0 || line.startsWith(' ') || line.startsWith('\t')) {
      bad(400, "请求头格式错误");
      return;
    }
    auto key = line.left(pos).trimmed().toLower();
    if (headers.contains(key)) {
      bad(400, "请求头字段重复");
      return;
    }
    headers.insert(key, line.mid(pos + 1).trimmed());
  }
  if (!headers.contains("host")) {
    bad(400, "请求缺少Host");
    return;
  }
  if (headers.contains("transfer-encoding")) {
    bad(400, "请使用Content-Length传输请求体");
    return;
  }
  qint64 length = 0;
  if (headers.contains("content-length")) {
    bool ok = false;
    length = headers.value("content-length").toLongLong(&ok);
    if (!ok || length < 0) {
      bad(400, "请求长度无效");
      return;
    }
  } else if (method == "POST") {
    bad(411, "请求缺少Content-Length");
    return;
  }
  if (length > 3 * 1024 * 1024) {
    bad(413, "请求内容过大");
    return;
  }
  if (bytes.size() < delimiter + 4 + length) {
    socket->setProperty("buffer", bytes);
    return;
  }
  if (bytes.size() > delimiter + 4 + length) {
    bad(400, "不支持连接上的请求流水线");
    return;
  }
  const auto origin = headers.value("origin");
  if (!origin.isEmpty()) {
    auto expected = "http://" + headers.value("host");
    if (origin != expected && origin != "http://127.0.0.1:5173"
        && origin != "http://localhost:5173") {
      bad(403, "来源不受允许");
      return;
    }
    socket->setProperty("origin", origin);
  }
  socket->setProperty("dispatched", true);
  socket->setProperty("buffer", QByteArray{});
  auto url = QUrl::fromEncoded(first[1], QUrl::StrictMode);
  if (!url.isValid() || !first[1].startsWith('/')
      || first[1].startsWith("//")) {
    bad(400, "请求路径无效");
    return;
  }
  auto path = url.path(QUrl::FullyDecoded);
  if (method == "OPTIONS") {
    reply(socket, 204, {});
    return;
  }
  if (method == "GET" && !path.startsWith("/api/")) {
    staticFile(socket, path);
    return;
  }
  QJsonObject input;
  bool raw = false;
  if (method == "GET" && path == "/api/health")
    input = {{"action", "health"}};
  else if (method == "GET" && path == "/api/dashboard") {
    input = {{"action", "dashboard"}};
    raw = true;
  } else if (method == "POST" && path == "/api/rpc") {
    if (!headers.value("content-type")
           .toLower()
           .startsWith("application/json")) {
      bad(415, "请求必须是JSON");
      return;
    }
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(bytes.mid(delimiter + 4, length),
                                       &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
      bad(400, "JSON格式无效");
      return;
    }
    input = doc.object();
  } else {
    bad(404, "接口不存在");
    return;
  }
  auto authorization = headers.value("authorization");
  auto token = authorization.startsWith("Bearer ")
               ? QString::fromUtf8(authorization.mid(7))
               : QString{};
  auto id = ++nextId_;
  pending_.insert(id, {socket, raw});
  emit request(id, input, token);
}
void HttpServer::respond(quint64 id, QJsonObject result) {
  if (!pending_.contains(id)) return;
  auto pending = pending_.take(id);
  if (!pending.socket) return;
  int code = 200;
  if (!result.value("ok").toBool()) {
    auto error = result.value("error").toObject().value("code").toString();
    code = error == "UNAUTHENTICATED" ? 401
         : error == "FORBIDDEN"       ? 403
         : error == "NOT_FOUND"       ? 404
                                      : 400;
  }
  if (pending.raw && code == 200) result = result.value("data").toObject();
  reply(pending.socket, code,
        QJsonDocument(result).toJson(QJsonDocument::Compact));
}
void HttpServer::reply(QTcpSocket *socket, int status, const QByteArray &body,
                       const QByteArray &type) {
  if (!socket || socket->property("replied").toBool()) return;
  socket->setProperty("replied", true);
  socket->setProperty("dispatched", true);
  for (auto *timer : socket->findChildren<QTimer *>()) timer->stop();
  QByteArray reason = status < 300 ? "OK" : "Error";
  QByteArray header = "HTTP/1.1 " + QByteArray::number(status) + " " + reason
                    + "\r\nContent-Type: " + type
                    + "\r\nContent-Length: " + QByteArray::number(body.size())
                    + "\r\nConnection: close\r\nCache-Control: "
                      "no-store\r\nX-Content-Type-Options: nosniff\r\n";
  auto origin = socket->property("origin").toByteArray();
  if (!origin.isEmpty())
    header += "Access-Control-Allow-Origin: " + origin
            + "\r\nVary: Origin\r\nAccess-Control-Allow-Methods: GET, POST, "
              "OPTIONS\r\nAccess-Control-Allow-Headers: Content-Type, "
              "Authorization\r\n";
  socket->write(header + "\r\n" + body);
  socket->disconnectFromHost();
}
void HttpServer::staticFile(QTcpSocket *socket, const QString &path) {
  auto reject = [this, socket] {
    reply(
      socket, 404,
      QJsonDocument(failure("NOT_FOUND", "页面或文件不存在，请先构建Web大屏"))
        .toJson(QJsonDocument::Compact));
  };
  if (path.contains(QChar('\0')) || path.contains('\\')
      || path.split('/').contains("..")) {
    reject();
    return;
  }
  bool upload = path.startsWith("/uploads/");
  auto base = QFileInfo(upload ? dataRoot_ + "/uploads" : webRoot_)
                .canonicalFilePath();
  auto relative = upload ? path.mid(9)
                         : (path == "/" ? QString("index.html") : path.mid(1));
  if (base.isEmpty() || relative.isEmpty()) {
    reject();
    return;
  }
  auto candidate = QFileInfo(QDir(base).filePath(relative)).canonicalFilePath();
  if (candidate.isEmpty() || !candidate.startsWith(base + "/")) {
    reject();
    return;
  }
  QFile file(candidate);
  if (!file.open(QIODevice::ReadOnly) || file.size() > 20 * 1024 * 1024) {
    reject();
    return;
  }
  auto mime = QMimeDatabase().mimeTypeForFile(candidate).name().toUtf8();
  if (candidate.endsWith(".js")) mime = "text/javascript";
  reply(socket, 200, file.readAll(), mime);
}
