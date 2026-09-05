#include "MobileController.h"
#include "UserMainWindow.h"

#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFontDatabase>
#include <QLabel>
#include <QPushButton>
#include <QQuickItem>
#include <QQuickWidget>
#include <QQuickWindow>
#include <QRandomGenerator>
#include <QSet>
#include <QSignalSpy>
#include <QWebEngineView>
#include <QtTest>
#include <memory>

// Opt-in integration test. Run against a disposable charging-server database.
// Buttons are activated through their QML clicked signals, exercising the
// actual screen bindings as well as the C++ controller and HTTP service.
class MobileFlowTest final : public QObject {
  Q_OBJECT

  QObject *item(UserMainWindow &window, const char *name) {
    QSet<QObject *> visited;
    std::function<QObject *(QObject *)> search =
      [&](QObject *object) -> QObject * {
      if (!object || visited.contains(object)) return nullptr;
      visited.insert(object);
      if (object->objectName() == QString::fromLatin1(name)) return object;
      for (auto *child : object->children()) {
        if (auto *result = search(child)) return result;
      }
      if (auto *visual = qobject_cast<QQuickItem *>(object)) {
        for (auto *child : visual->childItems()) {
          if (auto *result = search(child)) return result;
        }
      }
      return nullptr;
    };
    return search(window.quickView()->rootObject());
  }

  void click(UserMainWindow &window, const char *name) {
    auto *button = item(window, name);
    QVERIFY2(button, name);
    QVERIFY2(button->property("enabled").toBool(), name);
    QVERIFY(QMetaObject::invokeMethod(button, "clicked"));
  }

  void input(UserMainWindow &window, const char *name, const QString &value) {
    auto *field = item(window, name);
    QVERIFY2(field, name);
    QVERIFY(field->setProperty("text", value));
  }

  void capture(UserMainWindow &window, const QString &name) {
    const QString directory = qEnvironmentVariable("CHARGING_UI_ARTIFACT_DIR");
    if (directory.isEmpty()) return;
    QDir().mkpath(directory);
    if (auto *toast = item(window, "toastPopup"))
      QMetaObject::invokeMethod(toast, "close");
    QTest::qWait(100);
    QVERIFY(window.grab().save(QDir(directory).filePath(name + ".png")));
  }

private slots:
  void embeddedNavigation() {
    if (qEnvironmentVariable("CHARGING_UI_TEST_MAP") != "1")
      QSKIP("Set CHARGING_UI_TEST_MAP=1 with a graphical display for the live "
            "Tencent map check");
    UserMainWindow window;
    window.show();
    window.controller()->initialize();
    QTRY_VERIFY_WITH_TIMEOUT(window.controller()->stations().size() > 1, 10000);
    window.controller()->openNavigation(
      window.controller()->stations().at(1).toMap());
    auto *mode = window.findChild<QComboBox *>("routeMode");
    auto *route = window.findChild<QPushButton *>("routeButton");
    auto *map = window.findChild<QWebEngineView *>("tencentMapView");
    QVERIFY(mode);
    QVERIFY(route);
    QVERIFY(map);
    QCOMPARE(mode->itemData(0).toString(), QString("drive"));
    QCOMPARE(mode->itemData(1).toString(), QString("walk"));
    mode->setCurrentIndex(1);
    route->click();
    QTRY_VERIFY_WITH_TIMEOUT(map->title().contains("腾讯"), 30000);
    auto *mapStatus = window.findChild<QLabel *>("mapStatus");
    QVERIFY(mapStatus);
    QTRY_VERIFY_WITH_TIMEOUT(mapStatus->text().startsWith("腾讯地图"), 30000);
    QTest::qWait(4000);
    capture(window, "10-tencent-map");
    auto *back = window.findChild<QPushButton *>("mapBackButton");
    QVERIFY(back);
    back->click();
    QVERIFY(window.quickView()->isVisible());
  }

  void completeJourney() {
    if (qEnvironmentVariableIsEmpty("CHARGING_SERVER_URL"))
      QSKIP("Set CHARGING_SERVER_URL to a disposable test service");
    UserMainWindow window;
    window.show();
    auto *controller = window.controller();
    controller->initialize();
    const QString phone = '1'
                        + QString::number(
                            QRandomGenerator::global()->generate64()
                            % 10000000000ULL)
                            .rightJustified(10, '0');
    capture(window, "01-login");
    input(window, "phoneInput", phone);
    click(window, "loginButton");
    QTRY_VERIFY_WITH_TIMEOUT(controller->signedIn(), 10000);
    QTRY_VERIFY_WITH_TIMEOUT(!controller->busy(), 10000);
    QTRY_VERIFY_WITH_TIMEOUT(!controller->stations().isEmpty(), 10000);
    QCOMPARE(controller->page(), QString("home"));
    capture(window, "02-home");

    controller->selectTab("profile");
    click(window, "walletRechargeButton");
    QCOMPARE(controller->page(), QString("recharge"));
    input(window, "rechargeAmountInput", "0.01");
    click(window, "confirmRechargeButton");
    QTRY_COMPARE_WITH_TIMEOUT(
      controller->user().value("balanceCents").toLongLong(), 1LL, 10000);
    QTRY_COMPARE(controller->page(), QString("profile"));
    click(window, "walletRechargeButton");
    input(window, "rechargeAmountInput", "100.00");
    click(window, "confirmRechargeButton");
    QTRY_COMPARE_WITH_TIMEOUT(
      controller->user().value("balanceCents").toLongLong(), 10001LL, 10000);
    QTRY_COMPARE(controller->page(), QString("profile"));
    capture(window, "03-profile");

    controller->navigate("editProfile");
    input(window, "nicknameInput", "绿色出行测试员");
    click(window, "saveProfileButton");
    QTRY_COMPARE_WITH_TIMEOUT(controller->user().value("nickname").toString(),
                              QString("绿色出行测试员"), 10000);
    QTRY_COMPARE(controller->page(), QString("profile"));
    controller->selectTab("home");
    QTRY_VERIFY(!controller->loadingStations());
    int stationId = 0;
    for (const auto &entry : controller->stations()) {
      const auto station = entry.toMap();
      if (station.value("idleChargers").toInt() > 0) {
        stationId = station.value("id").toInt();
        break;
      }
    }
    QVERIFY(stationId > 0);
    controller->openStation(stationId);
    QTRY_COMPARE_WITH_TIMEOUT(controller->page(), QString("station"), 10000);
    capture(window, "04-station");
    int chargerId = 0;
    for (const auto &entry : controller->chargers()) {
      const auto charger = entry.toMap();
      if (charger.value("status") == "idle") {
        chargerId = charger.value("id").toInt();
        break;
      }
    }
    QVERIFY(chargerId > 0);
    const QByteArray reserveButton = "reserveCharger_"
                                   + QByteArray::number(chargerId);
    click(window, reserveButton.constData());
    QTRY_COMPARE_WITH_TIMEOUT(
      controller->activeOrder().value("status").toString(), QString("reserved"),
      10000);
    QTRY_COMPARE(controller->page(), QString("charge"));
    capture(window, "05-reserved");
    click(window, "cancelReservationButton");
    click(window, "confirmOrderActionButton");
    QTRY_COMPARE_WITH_TIMEOUT(
      controller->viewedOrder().value("status").toString(),
      QString("cancelled"), 10000);
    QTRY_VERIFY(controller->activeOrder().isEmpty());
    QTRY_VERIFY(!controller->busy());

    controller->selectTab("home");
    controller->openStation(stationId);
    QTRY_COMPARE_WITH_TIMEOUT(controller->page(), QString("station"), 10000);
    click(window, reserveButton.constData());
    QTRY_COMPARE_WITH_TIMEOUT(
      controller->activeOrder().value("status").toString(), QString("reserved"),
      10000);
    QTRY_VERIFY(!controller->busy());
    click(window, "startChargingButton");
    QTRY_COMPARE_WITH_TIMEOUT(
      controller->activeOrder().value("status").toString(), QString("charging"),
      10000);
    QTRY_VERIFY_WITH_TIMEOUT(
      controller->activeOrder().value("energyKwh").toDouble() > 0, 10000);
    capture(window, "06-charging");

    // A new client process/session must recover and intercept an unfinished
    // order.
    UserMainWindow recovery;
    recovery.show();
    recovery.controller()->initialize();
    QSignalSpy unfinished(recovery.controller(),
                          &MobileController::unfinishedOrder);
    input(recovery, "phoneInput", phone);
    click(recovery, "loginButton");
    QTRY_VERIFY_WITH_TIMEOUT(unfinished.count() == 1, 10000);
    QTRY_COMPARE(recovery.controller()->page(), QString("settlement"));
    click(recovery, "unfinishedOrderContinue");
    click(recovery, "stopChargingButton");
    click(recovery, "confirmOrderActionButton");
    QTRY_COMPARE_WITH_TIMEOUT(
      recovery.controller()->activeOrder().value("status").toString(),
      QString("pending_payment"), 10000);
    QTRY_VERIFY(!recovery.controller()->busy());
    capture(recovery, "07-settlement");
    click(recovery, "settleOrderButton");
    QTRY_COMPARE_WITH_TIMEOUT(
      recovery.controller()->viewedOrder().value("status").toString(),
      QString("paid"), 10000);
    QTRY_COMPARE(recovery.controller()->page(), QString("receipt"));
    const qint64 amount = recovery.controller()
                            ->viewedOrder()
                            .value("amountCents")
                            .toLongLong();
    QVERIFY(amount > 0);
    QTRY_COMPARE_WITH_TIMEOUT(
      recovery.controller()->user().value("balanceCents").toLongLong(),
      10001 - amount, 10000);
    capture(recovery, "08-receipt");
    click(recovery, "receiptDoneButton");
    QTRY_COMPARE(recovery.controller()->page(), QString("orders"));
    QTRY_COMPARE_WITH_TIMEOUT(recovery.controller()->orders().size(), 2, 10000);
    capture(recovery, "09-orders");
    recovery.controller()->selectTab("profile");
    click(recovery, "logoutButton");
    click(recovery, "confirmLogoutButton");
    QTRY_VERIFY_WITH_TIMEOUT(!recovery.controller()->signedIn(), 10000);
    QCOMPARE(recovery.controller()->page(), QString("login"));
  }
};

int main(int argc, char **argv) {
  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  QApplication application(argc, argv);
  if (QGuiApplication::platformName() == "offscreen")
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
  if (QFontDatabase::families().contains("Noto Sans CJK SC"))
    application.setFont(QFont("Noto Sans CJK SC", 10));
  MobileFlowTest test;
  return QTest::qExec(&test, argc, argv);
}

#include "MobileFlowTest.moc"
