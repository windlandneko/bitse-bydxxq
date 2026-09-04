#include "financialdialog.h"

#include <QDialogButtonBox>

#include "ui_financialdialog.h"

FinancialDialog::FinancialDialog(double rate, int years, bool compound,
                                 QWidget *parent)
    : QDialog(parent), ui(new Ui::FinancialDialog) {
    ui->setupUi(this);
    ui->rateSpin->setValue(rate);
    ui->yearsSpin->setValue(years);
    ui->compoundCheck->setChecked(compound);
    // 仅在确认时发布完整参数，取消操作不会改变主窗口状态。
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this] {
        emit parametersChanged(ui->rateSpin->value(), ui->yearsSpin->value(),
                               ui->compoundCheck->isChecked());
        accept();
    });
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

FinancialDialog::~FinancialDialog() { delete ui; }
