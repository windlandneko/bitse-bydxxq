#pragma once

#include <QMainWindow>
#include <QVariantMap>

class QLabel;
class MobileController;
class QComboBox;
class QQuickWidget;
class QStackedWidget;
class QWebEngineView;

class UserMainWindow final : public QMainWindow {
  Q_OBJECT

public:
  explicit UserMainWindow(QWidget *parent = nullptr);
  ~UserMainWindow() override;
  MobileController *controller() const { return m_controller; }
  QQuickWidget *quickView() const { return m_quick; }

private:
  void showNavigation(const QVariantMap &destination, const QString &originName,
                      double latitude, double longitude);
  void createMapPage();
  void loadRoute();

  MobileController *m_controller;
  QStackedWidget *m_pages;
  QQuickWidget *m_quick;
  QWidget *m_mapPage = nullptr;
  QWebEngineView *m_map = nullptr;
  QComboBox *m_routeMode = nullptr;
  QLabel *m_routeDescription = nullptr;
  QLabel *m_mapStatus = nullptr;
  QVariantMap m_destination;
  QString m_originName;
  double m_originLatitude = 0;
  double m_originLongitude = 0;
};
