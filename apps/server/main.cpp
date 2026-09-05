#include "HttpServer.h"
#include "Service.h"
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>
#include <QThread>
#include <csignal>

namespace {
volatile std::sig_atomic_t interrupted = 0;
void interrupt(int) { interrupted = 1; }
QFile *logFile = nullptr;
QMutex logMutex;
void logMessage(QtMsgType type, const QMessageLogContext &,
                const QString &message) {
  QMutexLocker lock(&logMutex);
  const QString line = utcNow() + " [" + QString::number(type) + "] " + message
                     + "\n";
  if (logFile) {
    logFile->write(line.toUtf8());
    logFile->flush();
  }
  fprintf(stderr, "%s", line.toLocal8Bit().constData());
  if (type == QtFatalMsg) abort();
}
} // namespace
int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  app.setApplicationName("charging-server");
  app.setApplicationVersion("1.0.0");
  QCommandLineParser parser;
  parser.setApplicationDescription("电动汽车充电平台统一业务服务");
  parser.addHelpOption();
  parser.addVersionOption();
  const QString root = QString::fromUtf8(CHARGING_SOURCE_DIR);
  parser.addOption({"data-dir", "数据、私有地图配置与日志目录", "path",
                    qEnvironmentVariable("CHARGING_DATA_DIR", root + "/data")});
  parser.addOption({"source-root", "机器学习项目目录的父目录", "path", root});
  parser.addOption({"web-root", "Web构建产物目录", "path", root + "/web/dist"});
  parser.addOption({"host", "监听地址", "address", "127.0.0.1"});
  parser.addOption({"port", "监听端口（0为自动分配）", "number", "8080"});
  parser.addOption({"time-scale", "模拟充电时间倍率1..3600", "number", "60"});
  parser.addOption({"no-seed", "仅初始化管理员和表结构，不写入演示数据"});
  parser.process(app);
  bool validPort, validScale;
  auto port = parser.value("port").toInt(&validPort),
       scale = parser.value("time-scale").toInt(&validScale);
  if (!validPort || port < 0 || port > 65535 || !validScale || scale < 1
      || scale > 3600) {
    fprintf(stderr, "端口或时间倍率无效\n");
    return 2;
  }
  auto data = QDir(parser.value("data-dir")).absolutePath();
  QDir().mkpath(data + "/logs");
  QFile log(data + "/logs/server-" + QDate::currentDate().toString(Qt::ISODate)
            + ".log");
  if (log.open(QIODevice::Append)) logFile = &log;
  qInstallMessageHandler(logMessage);
  HttpServer server(parser.value("web-root"), data);
  QThread worker;
  auto *service = new Service(data, parser.value("source-root"), scale,
                              !parser.isSet("no-seed"));
  service->moveToThread(&worker);
  QObject::connect(&worker, &QThread::started, service, &Service::initialize);
  QObject::connect(&worker, &QThread::finished, service, &QObject::deleteLater);
  QObject::connect(&server, &HttpServer::request, service, &Service::request,
                   Qt::QueuedConnection);
  QObject::connect(service, &Service::replied, &server, &HttpServer::respond,
                   Qt::QueuedConnection);
  QObject::connect(service, &Service::fatal, &app, [&](QString message) {
    qCritical().noquote() << message;
    app.exit(2);
  });
  QObject::connect(service, &Service::ready, &app, [&] {
    QHostAddress host(parser.value("host"));
    if (host.isNull() || !server.listen(host, quint16(port))) {
      qCritical().noquote() << "无法监听端口：" << server.errorString();
      app.exit(2);
      return;
    }
    QTextStream(stdout) << "LISTENING http://" << host.toString() << ":"
                        << server.serverPort() << "\n"
                        << Qt::flush;
    qInfo().noquote() << "SQLite service ready; data directory:" << data;
  });
  std::signal(SIGINT, interrupt);
  std::signal(SIGTERM, interrupt);
  QTimer signalTimer;
  QObject::connect(&signalTimer, &QTimer::timeout, &app, [&] {
    if (interrupted) app.quit();
  });
  signalTimer.start(200);
  worker.start();
  int result = app.exec();
  server.close();
  QMetaObject::invokeMethod(service, &Service::shutdown,
                            Qt::BlockingQueuedConnection);
  worker.quit();
  worker.wait();
  qInstallMessageHandler(nullptr);
  logFile = nullptr;
  return result;
}
