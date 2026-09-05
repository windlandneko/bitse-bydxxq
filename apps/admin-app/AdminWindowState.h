#pragma once

#include "AdminMainWindow.h"
#include "ApiClient.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QChart>
#include <QChartView>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDateTimeAxis>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QLineSeries>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QValueAxis>
#include <algorithm>
#include <cmath>
#include <functional>

class AdminMainWindow::Impl {
public:
  explicit Impl(AdminMainWindow *window);
  void call(QObject *owner, const QString &action, const QJsonObject &params,
            const std::function<void(QJsonValue)> &success,
            bool interactive = true, const std::function<void()> &failure = {});
  void read(const QString &action, const QJsonObject &params,
            const std::function<void(QJsonValue)> &success, bool interactive);
  void buildLogin();
  void buildWorkspace();
  QWidget *page(const QString &title, QVBoxLayout **layout);
  QLabel *metric(const QString &title, QHBoxLayout *row);
  void buildOverview();
  void refreshOverview(bool interactive);
  void buildStations();
  void refreshStations(bool interactive);
  void showStations();
  void stationEditor(const QJsonObject &existing);
  void chargerActions(QTableWidget *target, QHBoxLayout *toolbar,
                      QObject *owner, const std::function<void()> &refresh);
  void stationDetail(const QJsonObject &station);
  void buildChargers();
  void refreshChargers(bool interactive);
  void buildUsers();
  void refreshUsers(bool interactive);
  void userOrders(const QJsonObject &user);
  void buildOrders();
  void refreshOrders(bool interactive);
  void buildForecasts();
  static QJsonObject horizon(const QJsonArray &hours, int hour);
  static QString peaks(const QJsonArray &hours);
  void refreshForecasts(bool interactive);
  void refreshForecastStatus(bool interactive);
  void buildLogs();
  void refreshLogs(bool interactive);
  void refreshCurrent(bool interactive);
  void logoutNow();

  AdminMainWindow *w;
  ApiClient *api;
  QStackedWidget *central = nullptr;
  QStackedWidget *pages = nullptr;
  QLineEdit *serverUrl = nullptr;
  QLineEdit *username = nullptr;
  QLineEdit *password = nullptr;
  QPushButton *loginButton = nullptr;
  QLabel *loginError = nullptr;
  QLabel *identity = nullptr;
  QLabel *connection = nullptr;
  QLabel *updatedAt = nullptr;
  QCheckBox *autoRefresh = nullptr;
  QTimer *poll = nullptr;
  QTimer *forecastPoll = nullptr;
  int session = 0;
  bool loggedIn = false;
  QComboBox *trendDays = nullptr;
  QLabel *todayRevenue = nullptr;
  QLabel *monthRevenue = nullptr;
  QLabel *totalRevenue = nullptr;
  QLabel *todayOrders = nullptr;
  QChartView *revenueChart = nullptr;
  QTableWidget *statusTable = nullptr;
  QTableWidget *trendTable = nullptr;
  QTableWidget *stationTable = nullptr;
  QLabel *stationCount = nullptr;
  QLineEdit *stationSearch = nullptr;
  QJsonArray allStations;
  QTableWidget *chargerTable = nullptr;
  QLabel *chargerCount = nullptr;
  QComboBox *chargerStation = nullptr;
  QComboBox *chargerStatus = nullptr;
  QLineEdit *chargerSearch = nullptr;
  QTableWidget *userTable = nullptr;
  QLabel *userCount = nullptr;
  QLineEdit *userSearch = nullptr;
  QTableWidget *ordersTable = nullptr;
  QLabel *ordersCount = nullptr;
  QTableWidget *logsTable = nullptr;
  QLabel *logsCount = nullptr;
  QComboBox *forecastStation = nullptr;
  QTableWidget *stationForecasts = nullptr;
  QTableWidget *chargerForecasts = nullptr;
  QLabel *forecastMeta = nullptr;
  QLabel *forecastState = nullptr;
  QLabel *forecastWarnings = nullptr;
  QPushButton *runForecast = nullptr;
  bool forecastRunning = false;
  bool forecastRequestPending = false;
  QHash<QString, int> revisions;
};
