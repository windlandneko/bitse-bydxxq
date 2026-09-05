#include "AdminUi.h"
#include "AdminWindowState.h"

using namespace adminui;

void AdminMainWindow::Impl::buildStations() {
  QVBoxLayout *layout;
  page("充电站管理", &layout);
  auto *row = new QHBoxLayout;
  stationSearch = new QLineEdit;
  stationSearch->setPlaceholderText("按站名、区域或详细地址筛选");
  stationSearch->setClearButtonEnabled(true);
  row->addWidget(stationSearch, 1);
  auto *add = button("新增电站", row);
  add->setObjectName("addStationButton");
  auto *edit = button("编辑电站", row);
  auto *detail = button("站内电桩详情", row);
  layout->addLayout(row);
  stationTable = table({"电站 ID", "编号", "站名", "区域", "详细地址", "纬度",
                        "经度", "价格 (元/kWh)", "电桩总数", "空闲数量",
                        "在线率 (%)"},
                       "stationTable");
  stationTable->setColumnWidth(2, 200);
  stationTable->setColumnWidth(4, 280);
  layout->addWidget(stationTable, 1);
  stationCount = new QLabel("尚未加载电站");
  layout->addWidget(stationCount);
  onSearch(stationSearch, w, [this] {
    showStations();
  });
  QObject::connect(add, &QPushButton::clicked, w, [this] {
    stationEditor({});
  });
  QObject::connect(edit, &QPushButton::clicked, w, [this] {
    const auto item = selected(stationTable);
    if (!item.isEmpty()) stationEditor(item);
  });
  auto showDetail = [this] {
    const auto item = selected(stationTable);
    if (!item.isEmpty()) stationDetail(item);
  };
  QObject::connect(detail, &QPushButton::clicked, w, showDetail);
  QObject::connect(stationTable, &QTableWidget::cellDoubleClicked, w,
                   showDetail);
  auto enable = [this, edit, detail] {
    const bool hasSelection = !selected(stationTable).isEmpty();
    edit->setEnabled(hasSelection);
    detail->setEnabled(hasSelection);
  };
  QObject::connect(stationTable, &QTableWidget::itemSelectionChanged, w,
                   enable);
  enable();
}

void AdminMainWindow::Impl::refreshStations(bool interactive) {
  read(
    "admin.stations", {},
    [this](QJsonValue data) {
      allStations = data.toArray();
      showStations();
      for (auto *combo : {chargerStation, forecastStation}) {
        const auto selectedId = combo->currentData().toInt();
        const QSignalBlocker blocker(combo);
        combo->clear();
        combo->addItem("全部电站", 0);
        for (const auto &value : allStations) {
          const auto station = value.toObject();
          combo->addItem(station["name"].toString(), station["id"].toInt());
        }
        const int index = combo->findData(selectedId);
        combo->setCurrentIndex(qMax(0, index));
      }
    },
    interactive);
}

void AdminMainWindow::Impl::showStations() {
  const auto query = stationSearch->text().trimmed();
  QJsonArray visible;
  for (const auto &value : allStations) {
    const auto o = value.toObject();
    const auto searchable = o["name"].toString() + o["address"].toString()
                          + o["region"].toString() + o["code"].toString();
    if (query.isEmpty() || searchable.contains(query, Qt::CaseInsensitive))
      visible.append(o);
  }
  fill(stationTable, visible, [](const QJsonObject &o) -> QVariantList {
    return {o["id"].toInt(),
            o["code"].toString(),
            o["name"].toString(),
            o["region"].toString(),
            o["address"].toString(),
            number(o["latitude"], 6),
            number(o["longitude"], 6),
            money(o["priceCents"]),
            o["totalChargers"].toInt(),
            o["idleChargers"].toInt(),
            number(o["onlineRate"])};
  });
  stationCount->setText(QString("显示 %1 / %2 个电站 · 双击电站查看站内设备")
                          .arg(visible.size())
                          .arg(allStations.size()));
}

void AdminMainWindow::Impl::stationEditor(const QJsonObject &existing) {
  const bool editing = !existing.isEmpty();
  auto *dialog = new QDialog(w);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowTitle(editing ? "编辑电站" : "新增电站并自动创建电桩");
  dialog->setMinimumWidth(500);
  auto *layout = new QVBoxLayout(dialog);
  auto *form = new QFormLayout;
  auto *name = new QLineEdit(existing["name"].toString());
  name->setObjectName("stationName");
  name->setMaxLength(60);
  auto *address = new QLineEdit(existing["address"].toString());
  address->setObjectName("stationAddress");
  address->setMaxLength(200);
  auto *region = new QLineEdit(existing["region"].toString());
  region->setObjectName("stationRegion");
  region->setMaxLength(60);
  auto *latitude = new QDoubleSpinBox;
  latitude->setObjectName("stationLatitude");
  latitude->setRange(-90, 90);
  latitude->setDecimals(6);
  latitude->setValue(existing["latitude"].toDouble(31.2304));
  auto *longitude = new QDoubleSpinBox;
  longitude->setObjectName("stationLongitude");
  longitude->setRange(-180, 180);
  longitude->setDecimals(6);
  longitude->setValue(existing["longitude"].toDouble(121.4737));
  auto *price = new QDoubleSpinBox;
  price->setObjectName("stationPrice");
  price->setRange(0.01, 100);
  price->setDecimals(2);
  price->setSingleStep(0.1);
  price->setSuffix(" 元/kWh");
  price->setValue(existing["priceCents"].toDouble(120) / 100.0);
  auto *count = new QSpinBox(dialog);
  count->setVisible(!editing);
  count->setObjectName("stationChargerCount");
  count->setRange(1, 100);
  count->setValue(8);
  auto *type = new QComboBox(dialog);
  type->setVisible(!editing);
  type->addItem("混合快慢充（自动配置）", "");
  type->addItem("全部直流快充", "dc");
  type->addItem("全部交流慢充", "ac");
  auto *power = new QDoubleSpinBox(dialog);
  power->setVisible(!editing);
  power->setRange(1, 500);
  power->setDecimals(1);
  power->setSuffix(" kW");
  power->setValue(60);
  power->setEnabled(false);
  QObject::connect(
    type, qOverload<int>(&QComboBox::currentIndexChanged), dialog,
    [type, power] {
      power->setEnabled(!type->currentData().toString().isEmpty());
      power->setValue(type->currentData().toString() == "ac" ? 7 : 60);
    });
  form->addRow("站名 *", name);
  form->addRow("详细地址 *", address);
  form->addRow("所属区域 *", region);
  form->addRow("纬度 (−90 ~ 90) *", latitude);
  form->addRow("经度 (−180 ~ 180) *", longitude);
  form->addRow("充电单价 *", price);
  if (!editing) {
    form->addRow("电桩数量 *", count);
    form->addRow("电桩类型", type);
    form->addRow("单桩额定功率", power);
  }
  layout->addLayout(form);
  auto *tip = new QLabel(
    editing ? "修改价格仅用于后续开始充电的订单，已有充电订单按原价格结算。"
            : "保存时自动分配电站、电桩编号，并一次性创建指定数量的电桩。");
  tip->setWordWrap(true);
  layout->addWidget(tip);
  auto *error = new QLabel;
  error->setWordWrap(true);
  layout->addWidget(error);
  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save
                                       | QDialogButtonBox::Cancel);
  buttons->button(QDialogButtonBox::Save)->setText("保存");
  buttons->button(QDialogButtonBox::Save)->setObjectName("saveStationButton");
  buttons->button(QDialogButtonBox::Cancel)->setText("取消");
  layout->addWidget(buttons);
  QObject::connect(buttons, &QDialogButtonBox::rejected, dialog,
                   &QDialog::reject);
  QObject::connect(
    buttons, &QDialogButtonBox::accepted, dialog,
    [this, dialog, existing, name, address, region, latitude, longitude, price,
     count, type, power, buttons, error, editing] {
      if (name->text().trimmed().isEmpty()
          || address->text().trimmed().isEmpty()
          || region->text().trimmed().isEmpty()) {
        error->setText("请填写站名、详细地址和所属区域。");
        return;
      }
      QJsonObject params{{"name", name->text().trimmed()},
                         {"address", address->text().trimmed()},
                         {"region", region->text().trimmed()},
                         {"latitude", latitude->value()},
                         {"longitude", longitude->value()},
                         {"priceCents", qRound(price->value() * 100)}};
      if (editing)
        params["id"] = existing["id"];
      else {
        params["chargerCount"] = count->value();
        if (!type->currentData().toString().isEmpty()) {
          params["type"] = type->currentData().toString();
          params["powerKw"] = power->value();
        }
      }
      buttons->button(QDialogButtonBox::Save)->setEnabled(false);
      error->setText("正在保存…");
      call(
        dialog, "admin.station.save", params,
        [this, dialog](QJsonValue) {
          dialog->accept();
          w->statusBar()->showMessage("电站已保存", 6000);
          refreshStations(false);
        },
        true,
        [buttons, error] {
          buttons->button(QDialogButtonBox::Save)->setEnabled(true);
          error->setText("保存失败，请检查信息后重试。");
        });
    });
  dialog->open();
}

void AdminMainWindow::Impl::chargerActions(
  QTableWidget *target, QHBoxLayout *toolbar, QObject *owner,
  const std::function<void()> &refresh) {
  auto *restart = button("远程重启", toolbar);
  restart->setObjectName("restartChargerButton");
  auto *fault = button("标记故障", toolbar);
  fault->setObjectName("markChargerFaultButton");
  auto *restore = button("恢复可用", toolbar);
  restore->setObjectName("restoreChargerButton");
  auto update = [target, restart, fault, restore] {
    const auto item = selected(target);
    const auto status = item["status"].toString();
    const bool available = !item.isEmpty() && status != "charging"
                        && status != "reserved" && status != "restarting";
    restart->setEnabled(available);
    fault->setEnabled(available && status != "fault");
    restore->setEnabled(available && status != "idle");
  };
  QObject::connect(target, &QTableWidget::itemSelectionChanged, owner, update);
  update();
  QObject::connect(
    restart, &QPushButton::clicked, owner,
    [this, target, owner, refresh, restart] {
      const auto item = selected(target);
      if (item.isEmpty()) return;
      restart->setEnabled(false);
      call(
        owner, "admin.charger.restart", {{"chargerId", item["id"]}},
        [this, owner, refresh](QJsonValue data) {
          w->statusBar()->showMessage(
            data.toObject()["message"].toString("重启指令已发送"), 10000);
          refresh();
          const int epoch = session;
          QTimer::singleShot(3500, owner, [this, epoch, refresh] {
            if (loggedIn && session == epoch) refresh();
          });
        },
        true,
        [restart] {
          restart->setEnabled(true);
        });
    });
  for (auto entry : {qMakePair(fault, QString("fault")),
                     qMakePair(restore, QString("idle"))}) {
    QObject::connect(entry.first, &QPushButton::clicked, owner,
                     [this, target, owner, refresh, entry] {
                       const auto item = selected(target);
                       if (item.isEmpty()) return;
                       entry.first->setEnabled(false);
                       call(
                         owner, "admin.charger.status",
                         {{"chargerId", item["id"]}, {"status", entry.second}},
                         [this, refresh](QJsonValue) {
                           w->statusBar()->showMessage("电桩状态已更新", 6000);
                           refresh();
                         },
                         true,
                         [entry] {
                           entry.first->setEnabled(true);
                         });
                     });
  }
}

void AdminMainWindow::Impl::stationDetail(const QJsonObject &station) {
  auto *dialog = new QDialog(w);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowTitle(station["name"].toString() + " · 站内电桩");
  dialog->resize(1080, 560);
  auto *layout = new QVBoxLayout(dialog);
  layout->addWidget(heading(station["name"].toString()));
  auto *address = new QLabel(station["address"].toString());
  address->setWordWrap(true);
  layout->addWidget(address);
  auto *toolbar = new QHBoxLayout;
  auto *refreshButton = button("刷新设备", toolbar);
  toolbar->addStretch();
  layout->addLayout(toolbar);
  auto *details = table(chargerHeaders(), "stationChargerTable");
  layout->addWidget(details, 1);
  auto *summary = new QLabel("正在加载…");
  layout->addWidget(summary);
  auto refresh = [this, dialog, details, station, summary] {
    call(
      dialog, "admin.chargers", {{"stationId", station["id"]}},
      [details, summary](QJsonValue data) {
        fill(details, data.toArray(), chargerColumns);
        summary->setText(QString("共 %1 个电桩 · 每 5 秒自动刷新 · %2")
                           .arg(data.toArray().size())
                           .arg(QTime::currentTime().toString("HH:mm:ss")));
      },
      false);
  };
  chargerActions(details, toolbar, dialog, refresh);
  QObject::connect(refreshButton, &QPushButton::clicked, dialog, refresh);
  auto *timer = new QTimer(dialog);
  timer->setInterval(5000);
  QObject::connect(timer, &QTimer::timeout, dialog, refresh);
  timer->start();
  refresh();
  dialog->open();
}

void AdminMainWindow::Impl::buildChargers() {
  QVBoxLayout *layout;
  page("充电桩管理", &layout);
  auto *filters = new QHBoxLayout;
  chargerSearch = new QLineEdit;
  chargerSearch->setObjectName("chargerSearch");
  chargerSearch->setPlaceholderText("按电桩编号搜索");
  chargerSearch->setClearButtonEnabled(true);
  chargerStation = new QComboBox;
  chargerStation->addItem("全部电站", 0);
  chargerStatus = new QComboBox;
  chargerStatus->setObjectName("chargerStatusFilter");
  chargerStatus->addItem("全部状态", "");
  for (const auto &status :
       {"idle", "reserved", "charging", "fault", "offline", "restarting"})
    chargerStatus->addItem(state(status), status);
  filters->addWidget(chargerSearch, 1);
  filters->addWidget(chargerStation);
  filters->addWidget(chargerStatus);
  layout->addLayout(filters);
  auto *toolbar = new QHBoxLayout;
  toolbar->addWidget(new QLabel("预约、充电及重启中的电桩不能执行运维操作。"));
  toolbar->addStretch();
  layout->addLayout(toolbar);
  chargerTable = table(chargerHeaders(), "chargerTable");
  layout->addWidget(chargerTable, 1);
  chargerCount = new QLabel("尚未加载电桩");
  layout->addWidget(chargerCount);
  chargerActions(chargerTable, toolbar, w, [this] {
    refreshChargers(false);
  });
  onSearch(chargerSearch, w, [this] {
    if (loggedIn) refreshChargers(false);
  });
  for (auto *combo : {chargerStation, chargerStatus}) {
    QObject::connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), w,
                     [this] {
                       if (loggedIn) refreshChargers(false);
                     });
  }
}

void AdminMainWindow::Impl::refreshChargers(bool interactive) {
  QJsonObject params{{"query", chargerSearch->text().trimmed()}};
  if (chargerStation->currentData().toInt() > 0)
    params["stationId"] = chargerStation->currentData().toInt();
  if (!chargerStatus->currentData().toString().isEmpty())
    params["status"] = chargerStatus->currentData().toString();
  read(
    "admin.chargers", params,
    [this](QJsonValue data) {
      fill(chargerTable, data.toArray(), chargerColumns);
      chargerCount->setText(QString("当前筛选共 %1 个电桩 · 运维操作会写入日志")
                              .arg(data.toArray().size()));
    },
    interactive);
}
