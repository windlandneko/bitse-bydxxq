#include "MobileController.h"
#include "UserMainWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFontDatabase>
#include <QQuickItem>
#include <QQuickWidget>
#include <QQuickWindow>
#include <QTimer>
#include <memory>

int main(int argc, char *argv[]) {
  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  QApplication application(argc, argv);
  QApplication::setApplicationName("智充出行");
  QApplication::setOrganizationName("ChargingPlatform");
  if (QGuiApplication::platformName() == "offscreen")
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
  for (const auto &family : {"Noto Sans CJK SC", "WenQuanYi Micro Hei",
                             "Microsoft YaHei", "Noto Sans"}) {
    if (QFontDatabase::families().contains(QString::fromLatin1(family))) {
      application.setFont(QFont(QString::fromLatin1(family), 10));
      break;
    }
  }
  QCommandLineParser parser;
  parser.setApplicationDescription(
    "智充出行手机交互模拟客户端。业务由独立 charging-server 提供。");
  parser.addHelpOption();
  parser.addOption({"smoke-test", "启动并验证 QML 加载，随后退出。"});
  parser.addOption({"screenshot", "将当前窗口保存到指定 PNG 文件。", "path"});
  parser.addOption({"phone", "演示启动时登录指定手机号。", "phone"});
  parser.addOption(
    {"page", "截图前打开 home / orders / profile 页面。", "page", "home"});
  parser.process(application);
  if (!QStringList{"home", "orders", "profile"}.contains(
        parser.value("page"))) {
    qCritical() << "--page must be home, orders, or profile";
    return 2;
  }

  UserMainWindow window;
  if (window.quickView()->status() == QQuickWidget::Error) return 1;
  window.show();
  window.controller()->initialize();
  if (parser.isSet("phone")) {
    auto pageOpened = std::make_shared<bool>(false);
    QObject::connect(window.controller(), &MobileController::userChanged,
                     &application, [&, pageOpened] {
                       if (!*pageOpened && window.controller()->signedIn()) {
                         *pageOpened = true;
                         QTimer::singleShot(400, &window, [&] {
                           if (window.controller()->activeOrder().isEmpty())
                             window.controller()->selectTab(
                               parser.value("page"));
                         });
                       }
                     });
    window.controller()->login(parser.value("phone"));
  }
  if (parser.isSet("screenshot")) {
    const auto outputPath = parser.value("screenshot");
    QTimer::singleShot(2500, &window, [&, outputPath] {
      const bool saved = window.grab().save(outputPath);
      application.exit(saved ? 0 : 2);
    });
  } else if (parser.isSet("smoke-test")) {
    // Load every screen, including those behind login, so missing QML imports
    // or component syntax cannot hide until a user reaches a later step.
    const QStringList pages = {
      "login",  "home",    "station",  "charge",   "settlement", "receipt",
      "orders", "profile", "location", "recharge", "editProfile"};
    auto *timer = new QTimer(&application);
    auto index = std::make_shared<int>(0);
    QObject::connect(
      timer, &QTimer::timeout, &application, [&, timer, index, pages] {
        auto *loader = window.quickView()->rootObject()->findChild<QObject *>(
          "pageLoader");
        if (!loader || loader->property("status").toInt() == 3) {
          qCritical() << "QML page failed:" << window.controller()->page();
          application.exit(3);
          return;
        }
        if (*index == pages.size()) {
          qInfo() << "All" << pages.size()
                  << "mobile QML pages loaded successfully";
          application.quit();
          return;
        }
        window.controller()->navigate(pages.at((*index)++));
      });
    timer->start(100);
  }
  return application.exec();
}
