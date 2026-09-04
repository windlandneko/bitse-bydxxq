#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "calculatormodel.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

  private:
    void appendText(const QString &text);
    void calculate();
    void openFinancialDialog();
    void updateFinanceResult();

    Ui::MainWindow *ui;
    CalculatorModel model;
    double rate = 3.0;
    int years = 1;
    bool compound = false;
};
#endif // MAINWINDOW_H
