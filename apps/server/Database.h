#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QVariantList>
#include <stdexcept>

struct BusinessError : std::runtime_error {
  QString code;
  QString message;
  BusinessError(QString c, QString m)
      : std::runtime_error(m.toStdString()), code(std::move(c)),
        message(std::move(m)) {}
};

QString utcNow();
QString passwordHash(const QString &password, const QString &salt);
QJsonObject success(const QJsonValue &data = QJsonObject{});
QJsonObject failure(const QString &code, const QString &message);

class Database {
public:
  void open(const QString &directory, bool seed = true);
  void execute(const QString &sql, const QVariantList &values = {});
  QJsonArray rows(const QString &sql, const QVariantList &values = {});
  QJsonObject row(const QString &sql, const QVariantList &values = {});
  qint64 insert(const QString &sql, const QVariantList &values = {});
  void begin();
  void commit();
  void rollback();
  QString directory() const { return directory_; }

private:
  void seed();
  QSqlDatabase db_;
  QString directory_;
};

class Transaction {
public:
  explicit Transaction(Database &db) : db_(db) { db_.begin(); }
  ~Transaction() {
    if (!committed_) db_.rollback();
  }
  void commit() {
    db_.commit();
    committed_ = true;
  }

private:
  Database &db_;
  bool committed_ = false;
};
