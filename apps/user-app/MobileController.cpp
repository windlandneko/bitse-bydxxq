#include "MobileController.h"
#include "ApiClient.h"

#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QHash>
#include <QImageReader>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>
#include <QTimer>
#include <QUrl>
#include <QUuid>

MobileController::MobileController(QObject *parent)
    : QObject(parent), m_api(new ApiClient(this)),
      m_pollTimer(new QTimer(this)) {
  auto *clock = new QTimer(this);
  clock->setInterval(1000);
  connect(clock, &QTimer::timeout, this, [this] {
    if (m_activeOrder.value("status") == "reserved")
      emit reservationRemainingChanged();
  });
  clock->start();
  m_pollTimer->setInterval(2000);
  connect(m_pollTimer, &QTimer::timeout, this, [this] {
    if (!signedIn() || busy() || m_pollInFlight) return;
    fetchActive();
    if (++m_pollCount % 6 == 0) {
      if (m_page == "home") refreshStations();
      if (m_page == "station" && m_selectedStation)
        fetchStation(m_selectedStation, false);
      refreshProfile();
    }
  });
}

void MobileController::call(const QString &action, const QVariantMap &params,
                            Success success, bool foreground) {
  const int session = m_session;
  if (foreground) {
    clearError();
    ++m_pending;
    emit busyChanged();
  }
  m_api->call(
    action, QJsonObject::fromVariantMap(params),
    [this, session, success, foreground](QJsonValue value) {
      if (foreground) {
        --m_pending;
        emit busyChanged();
      }
      if (session != m_session) return;
      if (!m_online) {
        m_online = true;
        emit onlineChanged();
      }
      success(value);
    },
    [this, session, foreground, action](QString message) {
      if (foreground) {
        --m_pending;
        emit busyChanged();
      }
      if (session != m_session) return;
      if (action == "stations.list") {
        m_loadingStations = false;
        emit stationsChanged();
      }
      if (action == "orders.active") m_pollInFlight = false;
      if (!foreground && m_online) {
        m_online = false;
        emit onlineChanged();
      }
      if (foreground) setError(message);
    });
}

void MobileController::initialize() {
  call(
    "location.presets", {},
    [this](const QJsonValue &value) {
      m_presets = value.toVariant().toList();
      emit locationChanged();
      if (!m_presets.isEmpty())
        chooseLocation(0);
      else
        refreshStations();
    },
    false);
}

void MobileController::login(const QString &phone) {
  if (busy()) return;
  const QString number = phone.trimmed();
  if (!QRegularExpression("^1[0-9]{10}$").match(number).hasMatch()) {
    setError("请输入有效的 11 位手机号");
    return;
  }
  call("user.login", {{"phone", number}}, [this](const QJsonValue &value) {
    const auto result = value.toObject();
    setUser(result.value("user").toObject().toVariantMap());
    m_tab = "home";
    m_backStack.clear();
    setPage("home");
    if (m_presets.isEmpty())
      initialize();
    else
      refreshStations();
    fetchActive(true);
    m_pollTimer->start();
  });
}

void MobileController::logout() {
  if (busy()) return;
  // Send revocation with the current token, then clear the local session even
  // if connectivity has been lost. An old reply cannot clear a newer login.
  m_api->call(
    "auth.logout", {}, [](QJsonValue) {}, [](QString) {});
  m_api->setToken({});
  ++m_session;
  ++m_orderRevision;
  m_pollTimer->stop();
  m_pollInFlight = false;
  m_user.clear();
  m_orders.clear();
  m_activeOrder.clear();
  m_viewedOrder.clear();
  m_rechargeKey.clear();
  m_reservationKey.clear();
  m_backStack.clear();
  emit userChanged();
  emit ordersChanged();
  emit activeOrderChanged();
  emit reservationRemainingChanged();
  emit viewedOrderChanged();
  setPage("login");
}

void MobileController::setPage(const QString &page, bool push) {
  if (m_page == page) return;
  if (push) m_backStack.append(m_page);
  m_page = page;
  clearError();
  emit pageChanged();
}

void MobileController::selectTab(const QString &tab) {
  if (!signedIn() || busy()) return;
  m_tab = tab;
  m_backStack.clear();
  setPage(tab);
  emit pageChanged();
  if (tab == "home") refreshStations();
  if (tab == "orders") fetchOrders();
  if (tab == "profile") refreshProfile();
}

void MobileController::navigate(const QString &page) {
  if (busy()) return;
  setPage(page, true);
}

void MobileController::back() {
  if (busy()) return;
  if (!m_backStack.isEmpty())
    setPage(m_backStack.takeLast());
  else
    setPage(m_tab);
}

void MobileController::setError(const QString &message) {
  m_error = message;
  emit errorChanged();
}

void MobileController::clearError() {
  if (m_error.isEmpty()) return;
  m_error.clear();
  emit errorChanged();
}

void MobileController::setUser(const QVariantMap &user) {
  m_user = user;
  emit userChanged();
}

void MobileController::setActiveOrder(const QVariantMap &order) {
  const QString previousStatus = m_activeOrder.value("status").toString();
  m_activeOrder = order;
  emit activeOrderChanged();
  emit reservationRemainingChanged();
  if (!order.isEmpty() && m_viewedOrder.value("id") == order.value("id")) {
    m_viewedOrder = order;
    emit viewedOrderChanged();
  }
  if (previousStatus == "charging"
      && order.value("status") == "pending_payment") {
    emit notification("充电已结束，请结算");
    if (m_page == "charge") setPage("settlement");
  }
}

void MobileController::setQuery(const QString &value) {
  if (m_query == value) return;
  m_query = value;
  emit filtersChanged();
}

void MobileController::setSort(const QString &value) {
  if (m_sort == value) return;
  m_sort = value;
  emit filtersChanged();
  refreshStations();
}

void MobileController::setFastOnly(bool value) {
  if (m_fastOnly == value) return;
  m_fastOnly = value;
  emit filtersChanged();
  refreshStations();
}

QVariantMap MobileController::locationParams() const {
  if (!m_hasLocation) return {};
  return {{"latitude", m_latitude}, {"longitude", m_longitude}};
}

void MobileController::chooseLocation(int index) {
  if (index < 0 || index >= m_presets.size()) return;
  const auto preset = m_presets.at(index).toMap();
  m_latitude = preset.value("latitude").toDouble();
  m_longitude = preset.value("longitude").toDouble();
  m_locationName = preset.value("name").toString();
  m_hasLocation = true;
  emit locationChanged();
  refreshStations();
}

void MobileController::geocode(const QString &address) {
  if (busy()) return;
  if (address.trimmed().size() < 2) {
    setError("请输入完整地址，例如上海市人民广场");
    return;
  }
  call("location.geocode", {{"address", address.trimmed()}},
       [this](const QJsonValue &value) {
         const auto result = value.toObject();
         m_latitude = result.value("latitude").toDouble();
         m_longitude = result.value("longitude").toDouble();
         m_locationName = result.value("name").toString();
         m_hasLocation = true;
         emit locationChanged();
         back();
         refreshStations();
         emit notification("当前位置已更新");
       });
}

void MobileController::refreshStations() {
  auto params = locationParams();
  params.insert("query", m_query.trimmed());
  params.insert("sort", m_sort);
  params.insert("fastOnly", m_fastOnly);
  const int request = ++m_stationRequest;
  m_loadingStations = true;
  emit stationsChanged();
  call(
    "stations.list", params,
    [this, request](const QJsonValue &value) {
      if (request != m_stationRequest) return;
      m_stations = value.toVariant().toList();
      m_loadingStations = false;
      emit stationsChanged();
    },
    false);
}

void MobileController::fetchStation(int stationId, bool foreground) {
  auto params = locationParams();
  params.insert("stationId", stationId);
  call(
    "stations.detail", params,
    [this, stationId](const QJsonValue &value) {
      if (m_selectedStation != stationId) return;
      const auto result = value.toObject();
      m_station = result.value("station").toObject().toVariantMap();
      m_chargers = result.value("chargers").toVariant().toList();
      emit stationChanged();
      if (m_page == "home") setPage("station", true);
    },
    foreground);
}

void MobileController::openStation(int stationId) {
  if (busy()) return;
  m_selectedStation = stationId;
  m_station.clear();
  m_chargers.clear();
  emit stationChanged();
  fetchStation(stationId, true);
}

void MobileController::fetchActive(bool recover, std::function<void()> empty) {
  if (m_pollInFlight && !recover) return;
  if (recover) ++m_orderRevision;
  const int revision = m_orderRevision;
  m_pollInFlight = true;
  call(
    "orders.active", {},
    [this, recover, empty, revision](const QJsonValue &value) {
      m_pollInFlight = false;
      if (revision != m_orderRevision) return;
      const auto order = value.toObject().toVariantMap();
      const int oldId = m_activeOrder.value("id").toInt();
      const bool displayingActive = m_page == "charge"
                                 || m_page == "settlement";
      setActiveOrder(order);
      if (order.isEmpty()) {
        if (oldId && displayingActive) {
          call(
            "orders.get", {{"orderId", oldId}},
            [this](const QJsonValue &finished) {
              m_viewedOrder = finished.toObject().toVariantMap();
              emit viewedOrderChanged();
              setPage("receipt");
            },
            false);
        }
        if (empty) empty();
        return;
      }
      if (recover) {
        openActiveOrder();
        const auto status = order.value("status").toString();
        if (status == "charging" || status == "pending_payment")
          emit unfinishedOrder("有未完成的充电订单，请先结算");
        else
          emit notification("预约已恢复");
      }
    },
    recover);
}

void MobileController::reserve(int chargerId) {
  if (busy()) return;
  fetchActive(true, [this, chargerId] {
    if (m_user.value("balanceCents").toLongLong() <= 0) {
      emit notification("余额不足，请先充值");
      setPage("recharge", true);
      return;
    }
    if (m_reservationKey.isEmpty() || m_reservationCharger != chargerId) {
      m_reservationCharger = chargerId;
      m_reservationKey = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    call("orders.reserve",
         {{"chargerId", chargerId}, {"idempotencyKey", m_reservationKey}},
         [this](const QJsonValue &value) {
           setActiveOrder(value.toObject().toVariantMap());
           m_reservationKey.clear();
           setPage("charge", true);
           refreshStations();
           emit notification("预约成功");
         });
  });
}

void MobileController::openActiveOrder() {
  if (m_activeOrder.isEmpty()) return;
  const QString status = m_activeOrder.value("status").toString();
  setPage(status == "reserved" ? "charge" : "settlement", true);
}

void MobileController::orderAction(const QString &action) {
  if (busy() || m_activeOrder.isEmpty()) return;
  const int orderId = m_activeOrder.value("id").toInt();
  ++m_orderRevision;
  call(action, {{"orderId", orderId}}, [this, action](const QJsonValue &value) {
    const auto order = value.toObject().toVariantMap();
    const QString status = order.value("status").toString();
    if (status == "paid" || status == "cancelled") {
      m_viewedOrder = order;
      emit viewedOrderChanged();
      setActiveOrder({});
      setPage("receipt");
      refreshProfile();
      fetchOrders();
      refreshStations();
      emit notification(status == "paid" ? "支付成功" : "预约已取消");
    } else {
      setActiveOrder(order);
      setPage(status == "pending_payment" ? "settlement" : "charge");
      if (action == "orders.start") emit notification("已开始充电");
    }
  });
}

void MobileController::startCharging() { orderAction("orders.start"); }
void MobileController::cancelReservation() { orderAction("orders.cancel"); }
void MobileController::stopCharging() { orderAction("orders.stop"); }
void MobileController::settle() { orderAction("orders.settle"); }

void MobileController::fetchOrders() {
  call(
    "orders.list", {},
    [this](const QJsonValue &value) {
      m_orders = value.toVariant().toList();
      emit ordersChanged();
    },
    false);
}

void MobileController::openOrder(int orderId) {
  if (busy()) return;
  call("orders.get", {{"orderId", orderId}}, [this](const QJsonValue &value) {
    const auto order = value.toObject().toVariantMap();
    const auto status = order.value("status").toString();
    if (status == "reserved" || status == "charging"
        || status == "pending_payment") {
      setActiveOrder(order);
      openActiveOrder();
    } else {
      m_viewedOrder = order;
      emit viewedOrderChanged();
      setPage("receipt", true);
    }
  });
}

void MobileController::refreshProfile() {
  if (!signedIn()) return;
  call(
    "user.me", {},
    [this](const QJsonValue &value) {
      setUser(value.toObject().toVariantMap());
    },
    false);
}

void MobileController::refresh() {
  clearError();
  if (m_presets.isEmpty()) initialize();
  if (m_page == "home" || m_page == "login") refreshStations();
  if (m_page == "station") fetchStation(m_selectedStation, true);
  if (signedIn()) {
    fetchActive();
    refreshProfile();
    if (m_page == "orders") fetchOrders();
  }
}

void MobileController::recharge(const QString &amount) {
  if (busy()) return;
  const auto match = QRegularExpression("^([0-9]{1,5})(?:\\.([0-9]{1,2}))?$")
                       .match(amount.trimmed());
  if (!match.hasMatch()) {
    setError("请输入有效金额，最多保留两位小数");
    return;
  }
  const qint64 cents = match.captured(1).toLongLong() * 100
                     + match.captured(2).leftJustified(2, '0').toInt();
  if (cents < 1 || cents > 1000000) {
    setError("单次充值金额为 0.01 至 10,000 元");
    return;
  }
  if (m_rechargeKey.isEmpty() || m_rechargeAmount != cents) {
    m_rechargeAmount = cents;
    m_rechargeKey = QUuid::createUuid().toString(QUuid::WithoutBraces);
  }
  call("wallet.recharge",
       {{"amountCents", cents}, {"idempotencyKey", m_rechargeKey}},
       [this, cents](const QJsonValue &value) {
         setUser(value.toObject().toVariantMap());
         m_rechargeKey.clear();
         back();
         emit notification(QString("已到账 ¥%1").arg(cents / 100.0, 0, 'f', 2));
       });
}

void MobileController::updateNickname(const QString &nickname) {
  if (busy()) return;
  const auto name = nickname.trimmed();
  if (name.isEmpty() || name.size() > 24) {
    setError("昵称需要 1 至 24 个字符");
    return;
  }
  call("user.update", {{"nickname", name}}, [this](const QJsonValue &value) {
    setUser(value.toObject().toVariantMap());
    back();
    emit notification("个人信息已更新");
  });
}

void MobileController::chooseAvatar() {
  if (busy()) return;
  const QString path = QFileDialog::getOpenFileName(
    nullptr, "选择头像", {}, "图片 (*.png *.jpg *.jpeg)");
  if (path.isEmpty()) return;
  QFile image(path);
  if (!image.open(QIODevice::ReadOnly) || image.size() > 2 * 1024 * 1024) {
    setError("请选择大小不超过 2 MB 的 PNG 或 JPEG 图片");
    return;
  }
  QImageReader reader(path);
  const auto size = reader.size();
  if (!reader.canRead() || !size.isValid() || size.width() > 4096
      || size.height() > 4096) {
    setError("图片无法读取，请选择尺寸不超过 4096 × 4096 的图片");
    return;
  }
  call("user.update",
       {{"avatarBase64", QString::fromLatin1(image.readAll().toBase64())}},
       [this](const QJsonValue &value) {
         setUser(value.toObject().toVariantMap());
         emit notification("头像已更新");
       });
}

QString MobileController::avatarSource() const {
  const QString avatar = m_user.value("avatarUrl").toString();
  if (avatar.isEmpty()) return {};
  return QUrl(m_api->baseUrl() + '/').resolved(QUrl(avatar)).toString();
}

void MobileController::openNavigation(const QVariantMap &station) {
  if (!m_hasLocation) {
    setPage("location", true);
    emit notification("请先选择当前位置");
    return;
  }
  if (station.isEmpty()) return;
  emit navigationRequested(station, m_locationName, m_latitude, m_longitude);
}

QString MobileController::statusLabel(const QString &status) const {
  static const QHash<QString, QString> labels = {{"idle", "空闲"},
                                                 {"reserved", "已预约"},
                                                 {"charging", "充电中"},
                                                 {"fault", "故障"},
                                                 {"offline", "离线"},
                                                 {"restarting", "重启中"},
                                                 {"pending_payment", "待结算"},
                                                 {"paid", "已完成"},
                                                 {"cancelled", "已取消"}};
  return labels.value(status, status);
}

QString MobileController::formatTime(const QString &value) const {
  const auto time = QDateTime::fromString(value, Qt::ISODate);
  return time.isValid() ? time.toLocalTime().toString("yyyy-MM-dd HH:mm")
                        : QString("—");
}

QString MobileController::reservationRemaining() const {
  const auto expires = QDateTime::fromString(
    m_activeOrder.value("expiresAt").toString(), Qt::ISODate);
  const qint64 seconds = qMax<qint64>(
    0, QDateTime::currentDateTimeUtc().secsTo(expires));
  return QString("%1:%2")
    .arg(seconds / 60, 2, 10, QChar('0'))
    .arg(seconds % 60, 2, 10, QChar('0'));
}
