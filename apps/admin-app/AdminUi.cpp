#include "AdminUi.h"
#include "AdminWindowState.h"

namespace adminui {
namespace {
class SortableItem final : public QTableWidgetItem {
public:
  bool operator<(const QTableWidgetItem &other) const override {
    const auto left = data(Qt::UserRole + 1);
    const auto right = other.data(Qt::UserRole + 1);
    if (left.isValid() && right.isValid())
      return left.toDouble() < right.toDouble();
    return QTableWidgetItem::operator<(other);
  }
};
} // namespace

QString money(const QJsonValue &value) {
  return QStringLiteral("¥ %1").arg(value.toDouble() / 100.0, 0, 'f', 2);
}

QString number(const QJsonValue &value, int decimals) {
  return value.isDouble() ? QString::number(value.toDouble(), 'f', decimals)
                          : QStringLiteral("—");
}

QString timeText(const QJsonValue &value) {
  auto time = QDateTime::fromString(value.toString(), Qt::ISODateWithMs);
  if (!time.isValid())
    time = QDateTime::fromString(value.toString(), Qt::ISODate);
  return time.isValid() ? time.toLocalTime().toString("yyyy-MM-dd HH:mm:ss")
                        : QStringLiteral("—");
}

QString duration(const QJsonValue &value) {
  const auto seconds = qMax<qint64>(0, value.toVariant().toLongLong());
  return QStringLiteral("%1小时 %2分 %3秒")
    .arg(seconds / 3600)
    .arg(seconds / 60 % 60)
    .arg(seconds % 60);
}

QString state(const QString &value) {
  static const QMap<QString, QString> labels{
    {"idle", "闲置"},   {"reserved", "已预约"},  {"charging", "充电中"},
    {"fault", "故障"},  {"offline", "离线"},     {"restarting", "重启中"},
    {"active", "正常"}, {"frozen", "冻结"},      {"pending_payment", "待结算"},
    {"paid", "已支付"}, {"cancelled", "已取消"}, {"dc", "直流快充"},
    {"ac", "交流慢充"}};
  return labels.value(value, value);
}

QLabel *heading(const QString &text) {
  auto *label = new QLabel(text);
  auto font = label->font();
  font.setPointSize(font.pointSize() + 3);
  font.setBold(true);
  label->setFont(font);
  return label;
}

QTableWidget *table(const QStringList &headers, const QString &name) {
  auto *result = new QTableWidget(0, headers.size());
  result->setObjectName(name);
  result->setHorizontalHeaderLabels(headers);
  result->setAlternatingRowColors(true);
  result->setSelectionBehavior(QAbstractItemView::SelectRows);
  result->setSelectionMode(QAbstractItemView::SingleSelection);
  result->setEditTriggers(QAbstractItemView::NoEditTriggers);
  result->setSortingEnabled(true);
  result->sortItems(0, Qt::AscendingOrder);
  result->verticalHeader()->hide();
  result->horizontalHeader()->setStretchLastSection(true);
  result->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
  result->horizontalHeader()->setDefaultSectionSize(130);
  result->setMinimumHeight(140);
  return result;
}

QJsonObject selected(QTableWidget *tableWidget) {
  const auto row = tableWidget->currentRow();
  if (row < 0 || !tableWidget->item(row, 0)) return {};
  return QJsonObject::fromVariantMap(
    tableWidget->item(row, 0)->data(Qt::UserRole).toMap());
}

void fill(QTableWidget *target, const QJsonArray &rows,
          const Columns &columns) {
  const auto previousId = selected(target).value("id");
  const auto sortColumn = target->horizontalHeader()->sortIndicatorSection();
  const auto sortOrder = target->horizontalHeader()->sortIndicatorOrder();
  target->setSortingEnabled(false);
  target->setRowCount(rows.size());
  for (qsizetype index = 0; index < rows.size(); ++index) {
    const auto item = rows[index].toObject();
    const auto values = columns(item);
    for (int column = 0; column < target->columnCount(); ++column) {
      auto *cell = new SortableItem;
      cell->setData(Qt::DisplayRole, values.value(column));
      cell->setToolTip(values.value(column).toString());
      const auto text = values.value(column).toString();
      bool numeric = false;
      const double sortValue = (text.startsWith("¥ ") ? text.mid(2) : text)
                                 .toDouble(&numeric);
      if (numeric) cell->setData(Qt::UserRole + 1, sortValue);
      if (target->horizontalHeaderItem(column)->text().contains("时长")) {
        cell->setData(Qt::UserRole + 1,
                      item
                        .value(item.contains("chargingSeconds")
                                 ? "chargingSeconds"
                                 : "durationSeconds")
                        .toDouble());
      }
      if (column == 0) cell->setData(Qt::UserRole, item.toVariantMap());
      target->setItem(index, column, cell);
    }
  }
  target->setSortingEnabled(true);
  target->sortItems(sortColumn, sortOrder);
  target->clearSelection();
  target->setCurrentItem(nullptr);
  if (!previousId.isUndefined()) {
    for (int row = 0; row < target->rowCount(); ++row) {
      auto record = QJsonObject::fromVariantMap(
        target->item(row, 0)->data(Qt::UserRole).toMap());
      if (record.value("id") == previousId) {
        target->selectRow(row);
        break;
      }
    }
  }
}

QStringList chargerHeaders() {
  return {"电桩 ID",      "编号",         "所属电站",
          "类型",         "功率 (kW)",    "状态",
          "累计充电次数", "累计充电时长", "累计电量 (kWh)"};
}

QVariantList chargerColumns(const QJsonObject &o) {
  return {o["id"].toInt(),
          o["code"].toString(),
          o["stationName"].toString(),
          state(o["type"].toString()),
          o["powerKw"].toDouble(),
          state(o["status"].toString()),
          o["chargingCount"].toInt(),
          duration(o["chargingSeconds"]),
          number(o["energyKwh"], 2)};
}

QStringList orderHeaders() {
  return {"订单 ID",  "订单号", "用户 ID",    "电站",
          "电桩",     "状态",   "创建时间",   "开始时间",
          "结束时间", "时长",   "电量 (kWh)", "金额 (元)"};
}

QVariantList orderColumns(const QJsonObject &o) {
  return {o["id"].toInt(),
          o["orderNo"].toString(),
          o["userId"].toInt(),
          o["stationName"].toString(),
          o["chargerCode"].toString(),
          state(o["status"].toString()),
          timeText(o["createdAt"]),
          timeText(o["startedAt"]),
          timeText(o["endedAt"]),
          duration(o["durationSeconds"]),
          number(o["energyKwh"], 3),
          money(o["amountCents"])};
}

QPushButton *button(const QString &text, QHBoxLayout *row) {
  auto *result = new QPushButton(text);
  row->addWidget(result);
  return result;
}

void onSearch(QLineEdit *edit, QObject *owner,
              const std::function<void()> &action) {
  auto *delay = new QTimer(edit);
  delay->setSingleShot(true);
  delay->setInterval(350);
  QObject::connect(edit, &QLineEdit::textChanged, delay,
                   qOverload<>(&QTimer::start));
  QObject::connect(delay, &QTimer::timeout, owner, action);
  QObject::connect(edit, &QLineEdit::returnPressed, owner, [delay, action] {
    delay->stop();
    action();
  });
}
} // namespace adminui
