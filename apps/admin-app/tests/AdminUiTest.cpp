#include "AdminMainWindow.h"

#include <QChartView>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QLineSeries>
#include <QPointer>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <memory>

// Contract fixture: exercises the real asynchronous ApiClient and widgets
// through HTTP, including delayed responses that arrive after logout or dialog
// closure.
class RpcFixture : public QObject {
public:
  QTcpServer server;
  QJsonArray stations{{QJsonObject{{"id", 1},
                                   {"code", "ST001"},
                                   {"name", "人民广场站"},
                                   {"region", "黄浦区"},
                                   {"address", "人民大道 100 号"},
                                   {"latitude", 31.23},
                                   {"longitude", 121.47},
                                   {"priceCents", 123},
                                   {"totalChargers", 2},
                                   {"idleChargers", 1},
                                   {"onlineRate", 100}}}};
  QJsonArray chargers{QJsonObject{{"id", 1},
                                  {"stationId", 1},
                                  {"stationName", "人民广场站"},
                                  {"code", "DC001"},
                                  {"type", "dc"},
                                  {"powerKw", 60},
                                  {"status", "idle"},
                                  {"chargingCount", 7},
                                  {"chargingSeconds", 3661},
                                  {"energyKwh", 125.5}},
                      QJsonObject{{"id", 2},
                                  {"stationId", 1},
                                  {"stationName", "人民广场站"},
                                  {"code", "AC002"},
                                  {"type", "ac"},
                                  {"powerKw", 7},
                                  {"status", "charging"},
                                  {"chargingCount", 2},
                                  {"chargingSeconds", 120},
                                  {"energyKwh", 4}}};
  QJsonArray users{{QJsonObject{{"id", 1},
                                {"phone", "13800001234"},
                                {"nickname", "小明"},
                                {"balanceCents", 36109},
                                {"status", "active"},
                                {"createdAt", "2026-09-05T00:00:00Z"}}}};
  QMap<QString, int> requests;
  QMap<QString, QJsonObject> lastParams;
  bool running = false;
  bool holdUsers = false;
  QList<QPointer<QTcpSocket>> heldUsers;
  int unauthorized = 0;

  RpcFixture() {
    server.listen(QHostAddress::LocalHost, 0);
    connect(&server, &QTcpServer::newConnection, this, [this] {
      while (auto *socket = server.nextPendingConnection()) {
        connect(socket, &QTcpSocket::disconnected, socket,
                &QObject::deleteLater);
        connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
          auto bytes = socket->property("bytes").toByteArray()
                     + socket->readAll();
          socket->setProperty("bytes", bytes);
          const auto headerEnd = bytes.indexOf("\r\n\r\n");
          if (headerEnd < 0 || socket->property("handled").toBool()) return;
          int length = 0;
          for (const auto &header : bytes.left(headerEnd).split('\n')) {
            if (header.toLower().startsWith("content-length:"))
              length = header.mid(15).trimmed().toInt();
          }
          if (bytes.size() < headerEnd + 4 + length) return;
          socket->setProperty("handled", true);
          const auto rpc = QJsonDocument::fromJson(
                             bytes.mid(headerEnd + 4, length))
                             .object();
          const auto action = rpc["action"].toString();
          const auto params = rpc["params"].toObject();
          ++requests[action];
          lastParams[action] = params;
          if (action != "admin.login"
              && !bytes.left(headerEnd).contains("Bearer admin-token"))
            ++unauthorized;
          if (holdUsers && action == "admin.users") {
            heldUsers.append(socket);
            return;
          }
          if (action == "admin.login"
              && params["password"].toString() != "123456") {
            respond(socket,
                    {{"ok", false},
                     {"error", QJsonObject{{"code", "BAD_LOGIN"},
                                           {"message", "账号或密码错误"}}}});
            return;
          }
          respond(socket, {{"ok", true}, {"data", dispatch(action, params)}});
        });
      }
    });
  }

  QString url() const {
    return QString("http://127.0.0.1:%1").arg(server.serverPort());
  }

  static void respond(QTcpSocket *socket, const QJsonObject &object) {
    const auto bytes = QJsonDocument(object).toJson(QJsonDocument::Compact);
    socket->write(
      "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: "
      + QByteArray::number(bytes.size()) + "\r\nConnection: close\r\n\r\n"
      + bytes);
    socket->disconnectFromHost();
  }

  void releaseUsers() {
    holdUsers = false;
    for (auto socket : heldUsers) {
      if (socket) respond(socket, {{"ok", true}, {"data", users}});
    }
    heldUsers.clear();
  }

  QJsonValue dispatch(const QString &action, const QJsonObject &params) {
    if (action == "admin.login")
      return QJsonObject{{"token", "admin-token"}, {"username", "admin"}};
    if (action == "auth.logout") return QJsonObject{};
    if (action == "admin.overview") {
      QJsonArray trend;
      const int days = params["days"].toInt(7);
      for (int index = 0; index < days; ++index) {
        trend.append(QJsonObject{
          {"date",
           QDate(2026, 9, 5).addDays(index - days + 1).toString("yyyy-MM-dd")},
          {"revenueCents", (index + 1) * 123},
          {"orderCount", index + 1}});
      }
      return QJsonObject{
        {"todayRevenueCents", 123},
        {"monthRevenueCents", 45678},
        {"totalRevenueCents", 98765},
        {"todayOrders", 3},
        {"statusCounts", QJsonArray{QJsonObject{{"status", "idle"},
                                                {"label", "闲置"},
                                                {"count", 1},
                                                {"percent", 50}},
                                    QJsonObject{{"status", "charging"},
                                                {"label", "在用"},
                                                {"count", 1},
                                                {"percent", 50}}}},
        {"revenueTrend", trend}};
    }
    if (action == "admin.stations") return stations;
    if (action == "admin.station.save") {
      auto station = params;
      station["id"] = params["id"].toInt(2);
      station["code"] = "ST002";
      station["totalChargers"] = params["chargerCount"].toInt(2);
      station["idleChargers"] = station["totalChargers"];
      station["onlineRate"] = 100;
      if (params.contains("id"))
        stations[0] = station;
      else
        stations.append(station);
      return station;
    }
    if (action == "admin.chargers") {
      QJsonArray result;
      for (const auto &value : chargers) {
        auto row = value.toObject();
        if (params.contains("status") && row["status"] != params["status"])
          continue;
        result.append(row);
      }
      return result;
    }
    if (action == "admin.charger.restart" || action == "admin.charger.status") {
      for (int i = 0; i < chargers.size(); ++i) {
        auto item = chargers[i].toObject();
        if (item["id"] != params["chargerId"]) continue;
        item["status"] = action.endsWith("restart") ? QJsonValue("restarting")
                                                    : params["status"];
        chargers[i] = item;
      }
      return QJsonObject{{"message", "重启指令已发送"}};
    }
    if (action == "admin.users") return users;
    if (action == "admin.user.status") {
      auto user = users[0].toObject();
      user["status"] = params["status"];
      users[0] = user;
      return user;
    }
    if (action == "admin.orders")
      return QJsonArray{QJsonObject{{"id", 1},
                                    {"userId", 1},
                                    {"orderNo", "ORDER001"},
                                    {"stationName", "人民广场站"},
                                    {"chargerCode", "DC001"},
                                    {"status", "paid"},
                                    {"createdAt", "2026-09-05T00:00:00Z"},
                                    {"durationSeconds", 60},
                                    {"energyKwh", 1},
                                    {"amountCents", 123}}};
    if (action == "admin.logs")
      return QJsonArray{QJsonObject{{"id", 1},
                                    {"action", "admin.charger.restart"},
                                    {"target", "DC001"},
                                    {"detail", "管理员重启设备"},
                                    {"createdAt", "2026-09-05T00:00:00Z"}}};
    if (action == "forecasts.run") {
      running = true;
      return QJsonObject{{"running", true}};
    }
    if (action == "forecasts.status")
      return QJsonObject{{"running", running},
                         {"lastError", ""},
                         {"lastRunAt", "2026-09-05T00:00:00Z"}};
    if (action == "forecasts.list") {
      QJsonArray hours;
      for (int h = 1; h <= 24; ++h)
        hours.append(QJsonObject{{"hour", h},
                                 {"time", "2026-09-05T01:00:00Z"},
                                 {"loadKw", h * 1.5},
                                 {"availableChargers", 2},
                                 {"isPeak", h == 6}});
      return QJsonObject{
        {"generatedAt", "2026-09-05T00:00:00Z"},
        {"modelVersion", "baseline-v1"},
        {"source", "课程演示业务数据；样本不足，使用基线估计"},
        {"stations",
         QJsonArray{QJsonObject{
           {"stationId", 1},
           {"stationName", "人民广场站"},
           {"hours", hours},
           {"chargers",
            QJsonArray{QJsonObject{
              {"chargerId", 1}, {"code", "DC001"}, {"hours", hours}}}}}}}};
    }
    return QJsonObject{};
  }
};

class AdminUiTest : public QObject {
  Q_OBJECT
private:
  std::unique_ptr<RpcFixture> fixture;
  std::unique_ptr<AdminMainWindow> window;
  template <class T> T *widget(const char *name) {
    return window->findChild<T *>(name);
  }
  QPushButton *nav(int index) {
    return widget<QPushButton>(
      qPrintable("navigation" + QString::number(index)));
  }
  bool login() {
    widget<QLineEdit>("adminPassword")->setText("123456");
    widget<QPushButton>("adminLoginButton")->click();
    return QTest::qWaitFor([this] {
      return widget<QTableWidget>("trendTable")->rowCount() == 7;
    });
  }
private slots:
  void init() {
    fixture = std::make_unique<RpcFixture>();
    qputenv("CHARGING_SERVER_URL", fixture->url().toUtf8());
    window = std::make_unique<AdminMainWindow>();
    window->show();
  }

  void cleanup() {
    window.reset();
    fixture.reset();
  }

  void loginRevenueAndTrend() {
    widget<QLineEdit>("adminPassword")->setText("bad-password");
    widget<QPushButton>("adminLoginButton")->click();
    QTRY_COMPARE(widget<QLabel>("adminLoginError")->text(),
                 QString("账号或密码错误"));
    QVERIFY(login());
    QCOMPARE(widget<QTableWidget>("statusTable")->rowCount(), 2);
    QCOMPARE(widget<QTableWidget>("trendTable")->item(0, 1)->text(),
             QString("¥ 1.23"));
    auto *chart = widget<QChartView>("revenueChart")->chart();
    QCOMPARE(qobject_cast<QLineSeries *>(chart->series().first())->count(), 7);
    widget<QComboBox>("revenueTrendDays")->setCurrentIndex(1);
    QTRY_COMPARE(widget<QTableWidget>("trendTable")->rowCount(), 30);
    QCOMPARE(fixture->lastParams["admin.overview"]["days"].toInt(), 30);
    QCOMPARE(fixture->unauthorized, 0);
  }

  void stationCreationAndSafeDialogClosure() {
    QVERIFY(login());
    nav(1)->click();
    QTRY_COMPARE(widget<QTableWidget>("stationTable")->rowCount(), 1);
    widget<QPushButton>("addStationButton")->click();
    QTRY_VERIFY(widget<QLineEdit>("stationName"));
    widget<QLineEdit>("stationName")->setText("新校园站");
    widget<QLineEdit>("stationAddress")->setText("校园路 1 号");
    widget<QLineEdit>("stationRegion")->setText("校园区");
    widget<QDoubleSpinBox>("stationPrice")->setValue(1.29);
    widget<QSpinBox>("stationChargerCount")->setValue(3);
    widget<QPushButton>("saveStationButton")->click();
    QTRY_COMPARE(widget<QTableWidget>("stationTable")->rowCount(), 2);
    QCOMPARE(fixture->lastParams["admin.station.save"]["priceCents"].toInt(),
             129);
    QCOMPARE(fixture->lastParams["admin.station.save"]["chargerCount"].toInt(),
             3);
    QVERIFY(!fixture->lastParams["admin.station.save"].contains("type"));
    QTRY_VERIFY(!window->findChild<QDialog *>());
    auto *stations = widget<QTableWidget>("stationTable");
    stations->selectRow(0);
    stations->cellDoubleClicked(0, 0);
    QPointer<QDialog> detail = window->findChild<QDialog *>();
    QVERIFY(detail);
    detail->reject();
    QTRY_VERIFY(detail.isNull());
    QTRY_COMPARE(fixture->requests["admin.chargers"], 1);
    QCOMPARE(fixture->unauthorized, 0);
  }

  void devicesAndAccounts() {
    QVERIFY(login());
    nav(2)->click();
    auto *chargers = widget<QTableWidget>("chargerTable");
    QTRY_COMPARE(chargers->rowCount(), 2);
    chargers->selectRow(1);
    QVERIFY(!widget<QPushButton>("restartChargerButton")->isEnabled());
    chargers->selectRow(0);
    QVERIFY(widget<QPushButton>("restartChargerButton")->isEnabled());
    widget<QPushButton>("restartChargerButton")->click();
    QTRY_COMPARE(chargers->item(0, 5)->text(), QString("重启中"));
    QCOMPARE(fixture->lastParams["admin.charger.restart"]["chargerId"].toInt(),
             1);
    widget<QComboBox>("chargerStatusFilter")->setCurrentIndex(3);
    QTRY_COMPARE(chargers->rowCount(), 1);
    QCOMPARE(chargers->item(0, 1)->text(), QString("AC002"));
    nav(3)->click();
    auto *users = widget<QTableWidget>("userTable");
    QTRY_COMPARE(users->rowCount(), 1);
    QCOMPARE(users->item(0, 3)->text(), QString("¥ 361.09"));
    users->selectRow(0);
    widget<QPushButton>("freezeUserButton")->click();
    QTRY_COMPARE(users->item(0, 5)->text(), QString("冻结"));
    QVERIFY(widget<QPushButton>("unfreezeUserButton")->isEnabled());
    widget<QPushButton>("unfreezeUserButton")->click();
    QTRY_COMPARE(users->item(0, 5)->text(), QString("正常"));
    widget<QLineEdit>("userPhoneSearch")->setText("1234");
    QTRY_COMPARE(fixture->lastParams["admin.users"]["query"].toString(),
                 QString("1234"));
    QCOMPARE(fixture->unauthorized, 0);
  }

  void forecastAndLogs() {
    QVERIFY(login());
    nav(5)->click();
    auto *stations = widget<QTableWidget>("stationForecastTable");
    auto *chargers = widget<QTableWidget>("chargerForecastTable");
    QTRY_COMPARE(stations->rowCount(), 1);
    QCOMPARE(stations->item(0, 1)->text(), QString("1.50"));
    QCOMPARE(stations->item(0, 3)->text(), QString("9.00"));
    QCOMPARE(stations->item(0, 5)->text(), QString("36.00"));
    QCOMPARE(chargers->rowCount(), 1);
    QCOMPARE(chargers->item(0, 4)->text(), QString("36.00"));
    const int previous = fixture->requests["forecasts.list"];
    widget<QPushButton>("runForecastButton")->click();
    QTRY_VERIFY(fixture->running);
    QVERIFY(!widget<QPushButton>("runForecastButton")->isEnabled());
    fixture->running = false;
    QTRY_VERIFY_WITH_TIMEOUT(
      widget<QPushButton>("runForecastButton")->isEnabled(), 5000);
    QTRY_VERIFY(fixture->requests["forecasts.list"] > previous);
    nav(6)->click();
    QTRY_COMPARE(widget<QTableWidget>("logsTable")->rowCount(), 1);
    QCOMPARE(widget<QTableWidget>("logsTable")->item(0, 1)->text(),
             QString("远程重启"));
    QCOMPARE(fixture->unauthorized, 0);
  }

  void delayedResponseAfterLogoutIsIgnored() {
    QVERIFY(login());
    fixture->holdUsers = true;
    nav(3)->click();
    QTRY_COMPARE(fixture->heldUsers.size(), 1);
    widget<QPushButton>("adminLogoutButton")->click();
    QTRY_VERIFY(widget<QPushButton>("adminLoginButton")->isVisible());
    fixture->releaseUsers();
    QTest::qWait(150);
    QCOMPARE(widget<QTableWidget>("userTable")->rowCount(), 0);
    QCOMPARE(widget<QLineEdit>("adminPassword")->text(), QString());
    QTRY_COMPARE(fixture->requests["auth.logout"], 1);
    QCOMPARE(fixture->unauthorized, 0);
  }
};

QTEST_MAIN(AdminUiTest)
#include "AdminUiTest.moc"
