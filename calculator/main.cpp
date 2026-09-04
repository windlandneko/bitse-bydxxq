#include <QApplication>
#include <QFile>

#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    QFile styleFile(":/styles/calculator.qss");
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text))
        a.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    MainWindow w;
    w.show();
    return a.exec();
}
