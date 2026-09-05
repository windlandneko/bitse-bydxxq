#include "AdminUi.h"
#include "AdminWindowState.h"

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QPushButton>
#include <QStatusBar>
#include <QTableWidget>
#include <QVBoxLayout>

using namespace adminui;

void AdminMainWindow::Impl::buildUsers() {
  auto *layout = page("用户管理");
  auto *row = new QHBoxLayout;
  userSearch = new QLineEdit;
  userSearch->setObjectName("userPhoneSearch");
  userSearch->setPlaceholderText("搜索手机号（支持部分号码）");
  userSearch->setClearButtonEnabled(true);
  userSearch->setMaxLength(11);
  row->addWidget(userSearch, 1);
  auto *freeze = button("冻结账号", row);
  freeze->setObjectName("freezeUserButton");
  auto *unfreeze = button("解冻账号", row);
  unfreeze->setObjectName("unfreezeUserButton");
  auto *orders = button("查看用户订单", row);
  layout->addLayout(row);
  layout->addWidget(
    new QLabel("冻结后禁止新登录、预约和开始充电，已有订单仍可停止并结算。"));
  userTable = table(
    {"用户 ID", "手机号", "昵称", "钱包余额 (元)", "注册时间", "状态"},
    "userTable");
  userTable->setColumnWidth(4, 190);
  layout->addWidget(userTable, 1);
  userCount = new QLabel("尚未加载用户");
  layout->addWidget(userCount);
  onSearch(userSearch, w, [this] {
    if (loggedIn) refreshUsers(false);
  });
  auto update = [this, freeze, unfreeze, orders] {
    const auto item = selected(userTable);
    freeze->setEnabled(!item.isEmpty()
                       && item["status"].toString() == "active");
    unfreeze->setEnabled(!item.isEmpty()
                         && item["status"].toString() == "frozen");
    orders->setEnabled(!item.isEmpty());
  };
  QObject::connect(userTable, &QTableWidget::itemSelectionChanged, w, update);
  update();
  for (auto entry : {qMakePair(freeze, QString("frozen")),
                     qMakePair(unfreeze, QString("active"))}) {
    QObject::connect(entry.first, &QPushButton::clicked, w, [this, entry] {
      const auto user = selected(userTable);
      if (user.isEmpty()) return;
      entry.first->setEnabled(false);
      call(
        w, "admin.user.status",
        {{"userId", user["id"]}, {"status", entry.second}},
        [this](QJsonValue) {
          w->statusBar()->showMessage("用户账号状态已更新", 6000);
          refreshUsers(false);
        },
        true,
        [entry] {
          entry.first->setEnabled(true);
        });
    });
  }
  auto showOrders = [this] {
    const auto user = selected(userTable);
    if (!user.isEmpty()) userOrders(user);
  };
  QObject::connect(orders, &QPushButton::clicked, w, showOrders);
  QObject::connect(userTable, &QTableWidget::cellDoubleClicked, w, showOrders);
}

void AdminMainWindow::Impl::refreshUsers(bool interactive) {
  read(
    "admin.users", {{"query", userSearch->text().trimmed()}},
    [this](QJsonValue data) {
      fill(userTable, data.toArray(), [](const QJsonObject &o) -> QVariantList {
        return {o["id"].toInt(),          o["phone"].toString(),
                o["nickname"].toString(), money(o["balanceCents"]),
                timeText(o["createdAt"]), state(o["status"].toString())};
      });
      userCount->setText(
        QString("当前筛选共 %1 位用户 · 双击用户查看其订单记录")
          .arg(data.toArray().size()));
    },
    interactive);
}

void AdminMainWindow::Impl::userOrders(const QJsonObject &user) {
  auto *dialog = new QDialog(w);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowTitle(user["phone"].toString() + " · 用户订单");
  dialog->resize(1150, 560);
  auto *layout = new QVBoxLayout(dialog);
  layout->addWidget(
    heading(user["nickname"].toString() + " · " + user["phone"].toString()));
  auto *row = new QHBoxLayout;
  auto *summary = new QLabel("正在加载订单…");
  row->addWidget(summary, 1);
  auto *refreshButton = button("刷新订单", row);
  layout->addLayout(row);
  auto *details = table(orderHeaders(), "userOrdersTable");
  details->sortItems(0, Qt::DescendingOrder);
  layout->addWidget(details, 1);
  auto refresh = [this, dialog, user, details, summary] {
    call(dialog, "admin.orders", {{"userId", user["id"]}},
         [details, summary](QJsonValue data) {
           fill(details, data.toArray(), orderColumns);
           summary->setText(QString("共 %1 条订单 · 金额以实际结算为准")
                              .arg(data.toArray().size()));
         });
  };
  QObject::connect(refreshButton, &QPushButton::clicked, dialog, refresh);
  refresh();
  dialog->open();
}

void AdminMainWindow::Impl::buildOrders() {
  auto *layout = page("全平台订单记录");
  ordersTable = table(orderHeaders(), "ordersTable");
  ordersTable->sortItems(0, Qt::DescendingOrder);
  ordersTable->setColumnWidth(1, 220);
  for (int column : {6, 7, 8}) ordersTable->setColumnWidth(column, 190);
  layout->addWidget(ordersTable, 1);
  ordersCount = new QLabel("尚未加载订单");
  layout->addWidget(ordersCount);
}

void AdminMainWindow::Impl::refreshOrders(bool interactive) {
  read(
    "admin.orders", {},
    [this](QJsonValue data) {
      fill(ordersTable, data.toArray(), orderColumns);
      ordersCount->setText(QString("共 %1 条订单").arg(data.toArray().size()));
    },
    interactive);
}

void AdminMainWindow::Impl::buildLogs() {
  auto *layout = page("运维操作日志");
  logsTable = table({"日志 ID", "操作", "操作对象", "详情", "时间"},
                    "logsTable");
  logsTable->sortItems(0, Qt::DescendingOrder);
  logsTable->setColumnWidth(1, 180);
  logsTable->setColumnWidth(3, 380);
  logsTable->setColumnWidth(4, 190);
  layout->addWidget(logsTable, 1);
  logsCount = new QLabel("尚未加载日志");
  layout->addWidget(logsCount);
}

void AdminMainWindow::Impl::refreshLogs(bool interactive) {
  read(
    "admin.logs", {},
    [this](QJsonValue data) {
      fill(logsTable, data.toArray(),
           [](const QJsonObject &item) -> QVariantList {
             static const QMap<QString, QString> actions{
               {"admin.station.save", "保存电站"},
               {"admin.charger.restart", "远程重启"},
               {"admin.charger.status", "更改电桩状态"},
               {"admin.user.status", "更改用户状态"},
               {"forecasts.run", "运行负荷预测"},
               {"admin.login", "管理员登录"}};
             const auto action = item["action"].toString();
             return {item["id"].toInt(), actions.value(action, action),
                     item["target"].toVariant().toString(),
                     item["detail"].toString(), timeText(item["createdAt"])};
           });
      logsCount->setText(
        QString("共 %1 条运维记录").arg(data.toArray().size()));
    },
    interactive);
}
