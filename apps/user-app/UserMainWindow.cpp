#include "UserMainWindow.h"
#include "MobileController.h"

#include <QComboBox>
#include <QIcon>
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
    "QWidget#navigationPage { background: #f6f7f3; color: #192e24; }"
    "QPushButton { border: 2px solid transparent; border-radius: 16px; "
    "padding: 8px 16px; background: #e5efe7; color: #245b43; font-size: 14px; "
    "font-weight: 500; }"
    "QPushButton:pressed { background: #d4e4d5; }"
    "QPushButton:focus { border-color: #245b43; }"
    "QPushButton:disabled { background: #e2e7df; color: #879286; }"
    "QPushButton#mapBackButton { padding: 0; border-radius: 24px; background: "
    "transparent; }"
    "QPushButton#mapBackButton:pressed { background: #d4e4d5; }"
    "QPushButton#routeButton { background: #245b43; color: white; }"
    "QPushButton#routeButton:pressed { background: #174a33; }"
    "QPushButton#routeButton:focus { border-color: #d9ee89; }"
    "QComboBox { border: 1px solid #e0e7de; border-radius: 16px; padding: 8px "
    "16px; "
    "background: white; color: #192e24; font-size: 14px; }"
    "QComboBox:focus { border: 2px solid #245b43; }"
    "QComboBox::drop-down { width: 40px; border: none; }"
    "QComboBox::down-arrow { image: url(:/icons/chevron-down.svg); width: "
    "24px; height: 24px; }"
    "QComboBox QAbstractItemView::item { min-height: 48px; }"
    "QLabel { background: transparent; color: #192e24; }");
  auto *layout = new QVBoxLayout(m_mapPage);
  layout->setContentsMargins(16, 0, 16, 16);
  layout->setSpacing(16);
  auto *headerWidget = new QWidget(m_mapPage);
  headerWidget->setFixedHeight(64);
  auto *header = new QHBoxLayout(headerWidget);
  header->setContentsMargins(0, 0, 0, 0);
  header->setSpacing(8);
  auto *back = new QPushButton(m_mapPage);
  back->setObjectName("mapBackButton");
  back->setIcon(QIcon(":/icons/arrow-left.svg"));
  back->setIconSize(QSize(24, 24));
  back->setFixedSize(48, 48);
  back->setAccessibleName("返回电站页面");
  back->setToolTip("返回电站页面");
  auto *title = new QLabel("路线导航", m_mapPage);
  auto titleFont = title->font();
  titleFont.setPixelSize(22);
  titleFont.setWeight(QFont::DemiBold);
  title->setFont(titleFont);
  header->addWidget(back);
  header->addWidget(title, 1);
  layout->addWidget(headerWidget);
  connect(back, &QPushButton::clicked, this, [this] {
    m_map->stop();
    m_pages->setCurrentWidget(m_quick);
  });
  m_routeDescription = new QLabel(m_mapPage);
  m_routeDescription->setWordWrap(true);
  m_routeDescription->setTextFormat(Qt::PlainText);
  m_routeDescription->setStyleSheet("font-size: 14px;");
  layout->addWidget(m_routeDescription);
  auto *options = new QHBoxLayout;
  options->setSpacing(16);
  m_routeMode = new QComboBox(m_mapPage);
  m_routeMode->setObjectName("routeMode");
  m_routeMode->setMinimumHeight(48);
  m_routeMode->setAccessibleName("选择驾车或步行");
  m_routeMode->addItem("驾车出行", "drive");
  m_routeMode->addItem("步行前往", "walk");
  auto *route = new QPushButton("开始导航", m_mapPage);
  route->setObjectName("routeButton");
  route->setMinimumHeight(48);
  route->setAccessibleName("开始腾讯地图导航");
  options->addWidget(m_routeMode, 1);
  options->addWidget(route, 1);
  layout->addLayout(options);
  connect(route, &QPushButton::clicked, this, &UserMainWindow::loadRoute);
  m_mapStatus = new QLabel(m_mapPage);
  m_mapStatus->setObjectName("mapStatus");
  m_mapStatus->setWordWrap(true);
  m_mapStatus->setStyleSheet("font-size: 12px; color: #65746a;");
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
    m_mapStatus->setText("正在加载路线…");
    m_mapStatus->show();
  });
  connect(m_map, &QWebEngineView::loadFinished, this, [this](bool success) {
    m_mapStatus->setText(success ? QString()
                                 : QString("地图加载失败，请检查网络后重试。"));
    m_mapStatus->setVisible(!success);
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
  m_mapStatus->clear();
  m_mapStatus->hide();
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
