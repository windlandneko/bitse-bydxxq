#include "AdminUi.h"
#include "AdminWindowState.h"
#include "ApiClient.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QChart>
#include <QChartView>
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTableWidget>
#include <QTime>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

using namespace adminui;

AdminMainWindow::Impl::Impl(AdminMainWindow *window)
    : w(window), api(new ApiClient(window)) {
  w->setWindowTitle("东软充电 · 运营管理平台");
  w->resize(1280, 820);
  w->setMinimumSize(980, 660);
  central = new QStackedWidget;
  w->setCentralWidget(central);
  connection = new QLabel("尚未登录");
  w->statusBar()->addPermanentWidget(connection);
  buildLogin();
  buildWorkspace();
  central->setCurrentIndex(0);
  poll = new QTimer(w);
  poll->setInterval(15000);
  QObject::connect(poll, &QTimer::timeout, w, [this] {
    if (autoRefresh->isChecked()) refreshCurrent(false);
  });
  forecastPoll = new QTimer(w);
  forecastPoll->setInterval(3000);
  QObject::connect(forecastPoll, &QTimer::timeout, w, [this] {
    refreshForecastStatus(false);
  });
}

void AdminMainWindow::Impl::call(QObject *owner, const QString &action,
                                 const QJsonObject &params,
                                 const std::function<void(QJsonValue)> &success,
                                 bool interactive,
                                 const std::function<void()> &failure) {
  // Ignore responses after a dialog closes or the administrator logs out.
  QPointer<AdminMainWindow> window(w);
  QPointer<QObject> receiver(owner);
  const auto epoch = session;
  api->call(
    action, params,
    [this, window, receiver, epoch, success](QJsonValue data) {
      if (!window || !receiver || session != epoch) return;
      connection->setText("服务已连接");
      updatedAt->setText("上次同步："
                         + QTime::currentTime().toString("HH:mm:ss"));
      success(data);
    },
    [this, window, receiver, epoch, interactive, failure](QString message) {
      if (!window || !receiver || session != epoch) return;
      connection->setText("请求失败 · 可手动刷新");
      w->statusBar()->showMessage(message, 12000);
      if (failure) failure();
      if (interactive) QMessageBox::warning(w, "操作未完成", message);
    });
}

void AdminMainWindow::Impl::read(const QString &action,
                                 const QJsonObject &params,
                                 const std::function<void(QJsonValue)> &success,
                                 bool interactive) {
  const int revision = ++revisions[action];
  call(
    w, action, params,
    [this, action, revision, success](QJsonValue data) {
      if (revisions.value(action) == revision) success(data);
    },
    interactive);
}

void AdminMainWindow::Impl::buildLogin() {
  auto *page = new QWidget;
  auto *outer = new QVBoxLayout(page);
  outer->addStretch();
  auto *box = new QGroupBox("管理员登录");
  box->setMaximumWidth(500);
  auto *layout = new QVBoxLayout(box);
  layout->addWidget(heading("东软电动汽车充电运营平台"));
  auto *form = new QFormLayout;
  serverUrl = new QLineEdit(api->baseUrl());
  serverUrl->setObjectName("serverUrl");
  username = new QLineEdit("admin");
  username->setObjectName("adminUsername");
  password = new QLineEdit;
  password->setObjectName("adminPassword");
  password->setEchoMode(QLineEdit::Password);
  password->setPlaceholderText("请输入管理员密码");
  form->addRow("服务地址", serverUrl);
  form->addRow("账号", username);
  form->addRow("密码", password);
  layout->addLayout(form);
  loginError = new QLabel("默认账号：admin / 123456");
  loginError->setObjectName("adminLoginError");
  loginError->setWordWrap(true);
  layout->addWidget(loginError);
  loginButton = new QPushButton("登录");
  loginButton->setObjectName("adminLoginButton");
  loginButton->setDefault(true);
  layout->addWidget(loginButton);
  outer->addWidget(box, 0, Qt::AlignHCenter);
  outer->addStretch();
  central->addWidget(page);
  auto login = [this] {
    const QUrl url(serverUrl->text().trimmed());
    if (!url.isValid() || url.host().isEmpty()
        || (url.scheme() != "http" && url.scheme() != "https")) {
      loginError->setText("请输入有效的 http:// 或 https:// 服务地址");
      return;
    }
    if (username->text().trimmed().isEmpty() || password->text().isEmpty()) {
      loginError->setText("请输入账号和密码");
      return;
    }
    if (!loginButton->isEnabled()) return;
    loginButton->setEnabled(false);
    loginError->setText("正在验证账号…");
    api->setBaseUrl(serverUrl->text().trimmed());
    call(
      w, "admin.login",
      {{"username", username->text().trimmed()},
       {"password", password->text()}},
      [this](QJsonValue data) {
        api->setToken(data.toObject().value("token").toString());
        loggedIn = true;
        identity->setText(
          "管理员："
          + data.toObject().value("username").toString(username->text()));
        loginButton->setEnabled(true);
        password->clear();
        central->setCurrentIndex(1);
        pages->setCurrentIndex(0);
        refreshStations(false);
        refreshCurrent(true);
        poll->start();
      },
      false,
      [this] {
        loginButton->setEnabled(true);
        loginError->setText(w->statusBar()->currentMessage());
      });
  };
  QObject::connect(loginButton, &QPushButton::clicked, w, login);
  QObject::connect(password, &QLineEdit::returnPressed, w, login);
}

void AdminMainWindow::Impl::buildWorkspace() {
  auto *workspace = new QWidget;
  auto *root = new QHBoxLayout(workspace);
  auto *navigation = new QVBoxLayout;
  navigation->addWidget(heading("充电运营"));
  identity = new QLabel;
  navigation->addWidget(identity);
  auto *group = new QButtonGroup(w);
  const QStringList labels{"运营概览", "充电站管理", "充电桩管理", "用户管理",
                           "订单记录", "负荷预测",   "运维日志"};
  for (int i = 0; i < labels.size(); ++i) {
    auto *nav = new QPushButton(labels[i]);
    nav->setObjectName("navigation" + QString::number(i));
    nav->setCheckable(true);
    nav->setMinimumHeight(36);
    group->addButton(nav, i);
    navigation->addWidget(nav);
  }
  group->button(0)->setChecked(true);
  navigation->addStretch();
  auto *screen = new QPushButton("打开数据大屏");
  navigation->addWidget(screen);
  QObject::connect(screen, &QPushButton::clicked, w, [this] {
    QDesktopServices::openUrl(QUrl(api->baseUrl()).resolved(QUrl("/")));
  });
  auto *logout = new QPushButton("退出登录");
  logout->setObjectName("adminLogoutButton");
  navigation->addWidget(logout);
  QObject::connect(logout, &QPushButton::clicked, w, [this] {
    logoutNow();
  });
  root->addLayout(navigation);
  auto *body = new QVBoxLayout;
  auto *toolbar = new QHBoxLayout;
  updatedAt = new QLabel("尚未同步");
  toolbar->addWidget(updatedAt);
  toolbar->addStretch();
  autoRefresh = new QCheckBox("每 15 秒自动刷新当前页");
  autoRefresh->setChecked(true);
  toolbar->addWidget(autoRefresh);
  auto *refresh = button("刷新当前页", toolbar);
  refresh->setObjectName("refreshCurrentPage");
  QObject::connect(refresh, &QPushButton::clicked, w, [this] {
    refreshCurrent(true);
  });
  body->addLayout(toolbar);
  pages = new QStackedWidget;
  buildOverview();
  buildStations();
  buildChargers();
  buildUsers();
  buildOrders();
  buildForecasts();
  buildLogs();
  body->addWidget(pages, 1);
  root->addLayout(body, 1);
  central->addWidget(workspace);
  QObject::connect(group, &QButtonGroup::idClicked, w, [this](int index) {
    pages->setCurrentIndex(index);
    refreshCurrent(true);
  });
  QObject::connect(pages, &QStackedWidget::currentChanged, group,
                   [group](int index) {
                     group->button(index)->setChecked(true);
                   });
}

QVBoxLayout *AdminMainWindow::Impl::page(const QString &title) {
  auto *result = new QWidget;
  auto *layout = new QVBoxLayout(result);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->addWidget(heading(title));
  pages->addWidget(result);
  return layout;
}

QLabel *AdminMainWindow::Impl::metric(const QString &title, QHBoxLayout *row) {
  auto *box = new QGroupBox(title);
  auto *layout = new QVBoxLayout(box);
  auto *label = heading("—");
  layout->addWidget(label);
  row->addWidget(box, 1);
  return label;
}

void AdminMainWindow::Impl::refreshCurrent(bool interactive) {
  if (!loggedIn) return;
  switch (pages->currentIndex()) {
  case 0:
    refreshOverview(interactive);
    break;
  case 1:
    refreshStations(interactive);
    break;
  case 2:
    refreshChargers(interactive);
    break;
  case 3:
    refreshUsers(interactive);
    break;
  case 4:
    refreshOrders(interactive);
    break;
  case 5:
    refreshForecasts(interactive);
    refreshForecastStatus(false);
    break;
  case 6:
    refreshLogs(interactive);
    break;
  }
}

void AdminMainWindow::Impl::logoutNow() {
  poll->stop();
  forecastPoll->stop();
  loggedIn = false;
  forecastRunning = false;
  forecastRequestPending = false;
  ++session;
  // Send the current token before clearing it locally, even if logout fails.
  api->call(
    "auth.logout", {}, [](QJsonValue) {}, [](QString) {});
  api->setToken("");
  for (auto *dialog :
       w->findChildren<QDialog *>(QString(), Qt::FindDirectChildrenOnly))
    dialog->close();
  for (auto *widget :
       {statusTable, trendTable, stationTable, chargerTable, userTable,
        ordersTable, stationForecasts, chargerForecasts, logsTable})
    widget->setRowCount(0);
  for (auto *label : {todayRevenue, monthRevenue, totalRevenue, todayOrders})
    label->setText("—");
  allStations = {};
  for (auto *combo : {chargerStation, forecastStation}) {
    const QSignalBlocker blocker(combo);
    combo->clear();
    combo->addItem("全部电站", 0);
  }
  for (auto *edit : {stationSearch, chargerSearch, userSearch}) edit->clear();
  password->clear();
  auto *oldChart = revenueChart->chart();
  revenueChart->setChart(new QChart);
  delete oldChart;
  updatedAt->setText("尚未同步");
  connection->setText("已退出登录");
  loginError->setText("已退出登录");
  central->setCurrentIndex(0);
  password->setFocus();
}

AdminMainWindow::AdminMainWindow(QWidget *parent)
    : QMainWindow(parent), impl_(std::make_unique<Impl>(this)) {}

AdminMainWindow::~AdminMainWindow() = default;
