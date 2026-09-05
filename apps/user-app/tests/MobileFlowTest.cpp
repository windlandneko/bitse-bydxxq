#include "MobileController.h"
#include "UserMainWindow.h"

#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFontDatabase>
#include <QLabel>
#include <QMetaProperty>
#include <QPushButton>
#include <QQuickItem>
#include <QQuickWidget>
#include <QQuickWindow>
#include <QRandomGenerator>
#include <QSet>
#include <QSignalSpy>
#include <QWebEngineView>
#include <QtTest>

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
    // Loader releases the previous screen with deleteLater().
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
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

  void verifyLayout(UserMainWindow &window) {
    auto *root = window.quickView()->rootObject();
    auto *loader = item(window, "pageLoader");
    QVERIFY2(loader && loader->property("status").toInt() == 1,
             "Current QML page did not load");
    QSet<QQuickItem *> visited;
    std::function<void(QQuickItem *)> inspect = [&](QQuickItem *visual) {
      if (!visual || visited.contains(visual)) return;
      visited.insert(visual);
      if (visual->isVisible()
          && visual->window() == window.quickView()->quickWindow()) {
        if (visual->metaObject()->indexOfSignal("clicked()") >= 0) {
          QVERIFY2(visual->width() >= 48 && visual->height() >= 48,
                   qPrintable("Touch target smaller than 48: "
                              + visual->objectName()));
          bool canScrollHorizontally = false;
          for (auto *ancestor = visual->parentItem(); ancestor;
               ancestor = ancestor->parentItem()) {
            if (ancestor->property("contentX").isValid()
                && ancestor->property("contentWidth").toReal()
                     > ancestor->width() + 1) {
              canScrollHorizontally = true;
              break;
            }
          }
          const auto topLeft = visual->mapToItem(root, QPointF());
          QVERIFY2(canScrollHorizontally
                     || (topLeft.x() >= -1
                         && topLeft.x() + visual->width() <= root->width() + 1),
                   qPrintable("Action extends beyond viewport: "
                              + visual->objectName()));
        }
        if (visual->property("font").canConvert<QFont>()) {
          const int
            pixels = visual->property("font").value<QFont>().pixelSize();
          QVERIFY2(pixels < 0 || pixels >= 12,
                   "Rendered text is smaller than 12 px");
        }
        if (visual->objectName() == "stationPrice") {
          qreal baseline = -1;
          for (auto *label : visual->childItems()) {
            if (!label->isVisible() || !label->property("text").isValid())
              continue;
            const qreal actual = label->mapToItem(root, QPointF()).y()
                               + label->baselineOffset();
            if (baseline >= 0)
              QVERIFY2(qAbs(actual - baseline) < 1,
                       "Price labels do not share a baseline");
            baseline = actual;
          }
        }
      }
      for (auto *child : visual->childItems()) inspect(child);
    };
    inspect(root);
  }

  void capture(UserMainWindow &window, const QString &name) {
    if (auto *toast = item(window, "toastPopup"))
      QMetaObject::invokeMethod(toast, "close");
    QTest::qWait(100);
    verifyLayout(window);
    const QString directory = qEnvironmentVariable("CHARGING_UI_ARTIFACT_DIR");
    if (directory.isEmpty()) return;
    const QString size = QString::number(window.width()) + 'x'
                       + QString::number(window.height());
    const auto destination = QDir(directory).filePath(size);
    QDir().mkpath(destination);
    QVERIFY(window.grab().save(QDir(destination).filePath(name + ".png")));
  }

private slots:
  void embeddedNavigation() {
    if (qEnvironmentVariable("CHARGING_UI_TEST_MAP") != "1")
      QSKIP("Set CHARGING_UI_TEST_MAP=1 with a graphical display for the live "
            "Tencent map check");
    UserMainWindow window;
    window.resize(430, 860);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    window.controller()->initialize();
    QTRY_VERIFY_WITH_TIMEOUT(window.controller()->stations().size() > 1, 10000);
    window.controller()->navigate("home");
    const auto station = window.controller()->stations().at(1).toMap();
    const auto buttonName = "navigateStation_" + station.value("id").toString();
    auto *navigation = qobject_cast<QQuickItem *>(
      item(window, qPrintable(buttonName)));
    QVERIFY(navigation);
    auto *feed = qobject_cast<QQuickItem *>(item(window, "homePage"));
    QVERIFY(feed);
    const qreal targetY = navigation->mapToItem(feed, QPointF()).y();
    const qreal limit = feed->property("contentHeight").toReal()
                      - feed->height();
    feed->setProperty(
      "contentY", qBound(0.0, targetY - feed->height() / 2, qMax(0.0, limit)));
    QTest::qWait(100);
    const auto center = navigation->mapToItem(
      window.quickView()->rootObject(),
      QPointF(navigation->width() / 2, navigation->height() / 2));
    QTest::mouseClick(window.quickView(), Qt::LeftButton, Qt::NoModifier,
                      center.toPoint());
    QTRY_VERIFY(!window.quickView()->isVisible());
    // The nested navigation button must not also activate its station card.
    QCOMPARE(window.controller()->page(), QString("home"));
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
    QTRY_VERIFY_WITH_TIMEOUT(mapStatus->isHidden(), 30000);
    QTest::qWait(4000);
    for (const auto &size : {QSize(430, 860), QSize(360, 800)}) {
      window.resize(size);
      QTest::qWait(200);
      for (auto *control :
           {static_cast<QWidget *>(mode), static_cast<QWidget *>(route)}) {
        QVERIFY(control->width() >= 48 && control->height() >= 48);
        const int left = control->mapTo(&window, QPoint()).x();
        QVERIFY(left >= 16 && left + control->width() <= window.width() - 16);
      }
      capture(window, "10-tencent-map");
    }
    auto *back = window.findChild<QPushButton *>("mapBackButton");
    QVERIFY(back);
    back->click();
    QVERIFY(window.quickView()->isVisible());
  }

  void completeJourney_data() {
    QTest::addColumn<QSize>("windowSize");
    QTest::newRow("regular-phone") << QSize(430, 860);
    QTest::newRow("compact-phone") << QSize(360, 800);
  }

  void completeJourney() {
    QFETCH(QSize, windowSize);
    if (qEnvironmentVariableIsEmpty("CHARGING_SERVER_URL"))
      QSKIP("Set CHARGING_SERVER_URL to a disposable test service");
    UserMainWindow window;
    window.resize(windowSize);
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
    QVERIFY(!item(window, "stationCard_0"));
    auto *recommendation = item(window, "recommendedStationLoader");
    QVERIFY(recommendation);
    QVERIFY(!recommendation->property("active").toBool());
    QVERIFY(!recommendation->property("item").value<QObject *>());
    const auto
      firstId = controller->stations().first().toMap().value("id").toInt();
    auto *stationCard = item(
      window, qPrintable("stationCard_" + QString::number(firstId)));
    QVERIFY(stationCard);
    const int highlightedIndex = stationCard->metaObject()->indexOfProperty(
      "highlighted");
    QVERIFY(highlightedIndex >= 0);
    const auto highlighted = stationCard->metaObject()->property(
      highlightedIndex);
    QVERIFY(highlighted.isWritable());
    QVERIFY(highlighted.write(stationCard, true));
    QCOMPARE(stationCard->property("highlighted").toBool(), true);
    verifyLayout(window);
    QVERIFY(highlighted.write(stationCard, false));

    auto *homeTab = qobject_cast<QQuickItem *>(item(window, "tab_home"));
    QVERIFY(homeTab);
    QSignalSpy pageChanges(controller, &MobileController::pageChanged);
    const auto homeCenter = homeTab->mapToItem(
      window.quickView()->rootObject(),
      QPointF(homeTab->width() / 2, homeTab->height() / 2));
    QTest::mouseClick(window.quickView(), Qt::LeftButton, Qt::NoModifier,
                      homeCenter.toPoint());
    QCOMPARE(controller->page(), QString("home"));
    QCOMPARE(homeTab->property("highlighted").toBool(), true);
    QCOMPARE(pageChanges.count(), 0);

    controller->selectTab("profile");
    click(window, "walletRechargeButton");
    QCOMPARE(controller->page(), QString("recharge"));
    capture(window, "03a-recharge");
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
    capture(window, "03b-edit-profile");
    input(window, "nicknameInput", "绿色出行测试员");
    click(window, "saveProfileButton");
    QTRY_COMPARE_WITH_TIMEOUT(controller->user().value("nickname").toString(),
                              QString("绿色出行测试员"), 10000);
    QTRY_COMPARE(controller->page(), QString("profile"));
    controller->navigate("location");
    capture(window, "03c-location");
    controller->back();
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
    auto *countdown = item(window, "reservationCountdown");
    QVERIFY(countdown);
    QCOMPARE(countdown->property("text").toString(),
             controller->reservationRemaining());
    const auto remaining = countdown->property("text").toString();
    QTRY_VERIFY_WITH_TIMEOUT(
      countdown->property("text").toString() != remaining, 2000);
    controller->selectTab("home");
    click(window, "activeOrderBanner");
    QCOMPARE(controller->page(), QString("charge"));
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
    recovery.resize(windowSize);
    recovery.show();
    recovery.controller()->initialize();
    QSignalSpy unfinished(recovery.controller(),
                          &MobileController::unfinishedOrder);
    input(recovery, "phoneInput", phone);
    click(recovery, "loginButton");
    QTRY_VERIFY_WITH_TIMEOUT(unfinished.count() == 1, 10000);
    QTRY_COMPARE(recovery.controller()->page(), QString("settlement"));
    capture(recovery, "06a-unfinished-order");
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
    auto *ordersPage = item(recovery, "ordersPage");
    QVERIFY(ordersPage);
    const auto paidCardName = "orderCard_"
                            + recovery.controller()
                                ->viewedOrder()
                                .value("id")
                                .toString();
    QVERIFY(ordersPage->setProperty("filter", "active"));
    QVERIFY(!item(recovery, qPrintable(paidCardName)));
    QVERIFY(ordersPage->setProperty("filter", "paid"));
    QVERIFY(item(recovery, qPrintable(paidCardName)));
    QVERIFY(ordersPage->setProperty("filter", "all"));
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
