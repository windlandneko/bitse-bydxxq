QT += widgets
CONFIG += c++17
lessThan(QT_MAJOR_VERSION, 6): error("Require Qt 6")
TEMPLATE = app
TARGET = calculator
SOURCES += main.cpp mainwindow.cpp calculatormodel.cpp draggablebutton.cpp financialdialog.cpp
HEADERS += mainwindow.h calculatormodel.h draggablebutton.h financialdialog.h
FORMS += mainwindow.ui financialdialog.ui
RESOURCES += resources.qrc
