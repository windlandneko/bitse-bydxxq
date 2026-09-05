#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QVariantList>
#include <functional>

class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace adminui {
QString money(const QJsonValue &value);
QString number(const QJsonValue &value, int decimals = 1);
QString timeText(const QJsonValue &value);
QString duration(const QJsonValue &value);
QString state(const QString &value);
QLabel *heading(const QString &text);
QTableWidget *table(const QStringList &headers, const QString &name);
QJsonObject selected(QTableWidget *tableWidget);
using Columns = std::function<QVariantList(const QJsonObject &)>;
void fill(QTableWidget *target, const QJsonArray &rows, const Columns &columns);
QStringList chargerHeaders();
QVariantList chargerColumns(const QJsonObject &o);
QStringList orderHeaders();
QVariantList orderColumns(const QJsonObject &o);
QPushButton *button(const QString &text, QHBoxLayout *row);
void onSearch(QLineEdit *edit, QObject *owner,
              const std::function<void()> &action);
} // namespace adminui
