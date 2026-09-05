#include "AdminMainWindow.h"

#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication application(argc, argv);
  QApplication::setApplicationName("充电运营管理端");
  AdminMainWindow window;
  window.show();
  return application.exec();
}
