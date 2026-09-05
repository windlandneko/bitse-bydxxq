#include "UserMainWindow.h"
#include "MobileController.h"

#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QQmlContext>
#include <QQuickWidget>
#include <QStackedWidget>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineView>

namespace {
// URI route planning can offer an installed mobile application. Keep the
// navigation in this window and never launch external application schemes.
class EmbeddedMapPage final : public QWebEnginePage {
public:
  EmbeddedMapPage(QWebEngineProfile *profile, QObject *parent)
      : QWebEnginePage(profile, parent) {}

protected:
  bool acceptNavigationRequest(const QUrl &url, NavigationType, bool) override {
    return url.scheme() == "https" || url.scheme() == "http"
        || url.scheme() == "about";
  }
};
} // namespace

UserMainWindow::UserMainWindow(QWidget *parent)
    : QMainWindow(parent), m_controller(new MobileController(this)),
      m_pages(new QStackedWidget(this)), m_quick(new QQuickWidget(this)) {
  setObjectName("mobileWindow");
  setWindowTitle("智充出行 · 电动汽车充电服务");
  resize(430, 860);
  setMinimumSize(360, 680);
  setMaximumWidth(560);
  m_quick->setObjectName("mobileQuickView");
  m_quick->setResizeMode(QQuickWidget::SizeRootObjectToView);
  m_quick->setClearColor(QColor("#f6f7f3"));
  m_quick->rootContext()->setContextProperty("mobile", m_controller);
  m_quick->setSource(QUrl("qrc:/qml/Main.qml"));
  m_pages->addWidget(m_quick);
  setCentralWidget(m_pages);
  connect(m_controller, &MobileController::navigationRequested, this,
          &UserMainWindow::showNavigation);
}

UserMainWindow::~UserMainWindow() {
  // QML bindings must disappear before their context controller is destroyed.
  delete m_quick;
}

void UserMainWindow::createMapPage() {
  m_mapPage = new QWidget(m_pages);
  m_mapPage->setObjectName("navigationPage");
  m_mapPage->setStyleSheet(
    "QWidget#navigationPage { background: #f6f7f3; color: #182821; }"
    "QPushButton { border: none; border-radius: 12px; padding: 12px; "
    "background: #e6eee7; color: #225643; font-weight: 600; }"
    "QPushButton#routeButton { background: #245b43; color: white; }"
    "QComboBox { border: 1px solid #dce5dd; border-radius: 10px; padding: "
    "10px; background: white; }"
    "QLabel { background: transparent; }");
  auto *layout = new QVBoxLayout(m_mapPage);
  layout->setContentsMargins(18, 18, 18, 14);
  layout->setSpacing(12);
  auto *header = new QHBoxLayout;
  auto *back = new QPushButton("‹ 返回", m_mapPage);
  back->setObjectName("mapBackButton");
  auto *title = new QLabel("路线导航", m_mapPage);
  auto titleFont = title->font();
  titleFont.setPointSize(16);
  titleFont.setBold(true);
  title->setFont(titleFont);
  header->addWidget(back);
  header->addStretch();
  header->addWidget(title);
  header->addStretch();
  layout->addLayout(header);
  connect(back, &QPushButton::clicked, this, [this] {
    m_map->stop();
    m_pages->setCurrentWidget(m_quick);
  });
  m_routeDescription = new QLabel(m_mapPage);
  m_routeDescription->setWordWrap(true);
  m_routeDescription->setTextFormat(Qt::PlainText);
  m_routeDescription->setStyleSheet("font-size: 14px; line-height: 1.6;");
  layout->addWidget(m_routeDescription);
  auto *options = new QHBoxLayout;
  m_routeMode = new QComboBox(m_mapPage);
  m_routeMode->setObjectName("routeMode");
  m_routeMode->addItem("驾车出行", "drive");
  m_routeMode->addItem("步行前往", "walk");
  auto *route = new QPushButton("开始导航", m_mapPage);
  route->setObjectName("routeButton");
  options->addWidget(m_routeMode, 1);
  options->addWidget(route, 1);
  layout->addLayout(options);
  connect(route, &QPushButton::clicked, this, &UserMainWindow::loadRoute);
  m_mapStatus = new QLabel("选择出行方式，点击开始导航", m_mapPage);
  m_mapStatus->setObjectName("mapStatus");
  m_mapStatus->setWordWrap(true);
  m_mapStatus->setStyleSheet("font-size: 12px; color: #6c7b70;");
  layout->addWidget(m_mapStatus);
  m_map = new QWebEngineView(m_mapPage);
  m_map->setObjectName("tencentMapView");
  auto *profile = new QWebEngineProfile(this);
  profile->setHttpUserAgent(
    "Mozilla/5.0 (iPhone; CPU iPhone OS 16_0 like Mac OS X) "
    "AppleWebKit/605.1.15 (KHTML, like Gecko) Version/16.0 "
    "Mobile/15E148 Safari/604.1");
  m_map->setPage(new EmbeddedMapPage(profile, m_map));
  m_map->settings()->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows,
                                  false);
  m_map->settings()->setAttribute(
    QWebEngineSettings::LocalContentCanAccessFileUrls, false);
  layout->addWidget(m_map, 1);
  connect(m_map, &QWebEngineView::loadStarted, this, [this] {
    m_mapStatus->setText("正在加载腾讯地图路线…");
  });
  connect(m_map, &QWebEngineView::loadFinished, this, [this](bool success) {
    m_mapStatus->setText(
      success ? "腾讯地图 · 路线仅供出行参考"
              : "地图暂时无法加载，请检查网络后点击开始导航重试。");
  });
  m_pages->addWidget(m_mapPage);
}

void UserMainWindow::showNavigation(const QVariantMap &destination,
                                    const QString &originName, double latitude,
                                    double longitude) {
  if (!m_mapPage) createMapPage();
  m_destination = destination;
  m_originName = originName;
  m_originLatitude = latitude;
  m_originLongitude = longitude;
  m_routeDescription->setText("起点  " + originName + "\n终点  "
                              + destination.value("name").toString());
  m_map->setUrl(QUrl("about:blank"));
  m_mapStatus->setText("选择驾车或步行，点击开始导航");
  m_pages->setCurrentWidget(m_mapPage);
}

void UserMainWindow::loadRoute() {
  QUrl url("https://apis.map.qq.com/uri/v1/routeplan");
  QUrlQuery query;
  query.addQueryItem("type", m_routeMode->currentData().toString());
  query.addQueryItem("from", m_originName);
  query.addQueryItem("fromcoord",
                     QString::number(m_originLatitude, 'f', 6) + ','
                       + QString::number(m_originLongitude, 'f', 6));
  query.addQueryItem("to", m_destination.value("name").toString());
  query.addQueryItem(
    "tocoord",
    QString::number(m_destination.value("latitude").toDouble(), 'f', 6) + ','
      + QString::number(m_destination.value("longitude").toDouble(), 'f', 6));
  query.addQueryItem("policy", "0");
  query.addQueryItem("referer", "智充出行");
  url.setQuery(query);
  m_map->setUrl(url);
}
