#ifndef FINANCIALDIALOG_H
#define FINANCIALDIALOG_H

#include <QDialog>

namespace Ui {
class FinancialDialog;
}

class FinancialDialog : public QDialog {
    Q_OBJECT
  public:
    FinancialDialog(double rate, int years, bool compound,
                    QWidget *parent = nullptr);
    ~FinancialDialog();

  signals:
    void parametersChanged(double rate, int years, bool compound);

  private:
    Ui::FinancialDialog *ui;
};

#endif
