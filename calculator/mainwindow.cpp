#include "mainwindow.h"

#include <QApplication>
#include <QButtonGroup>
#include <QIcon>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QSizePolicy>

#include "./ui_mainwindow.h"
#include "draggablebutton.h"
#include "financialdialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setWindowIcon(QIcon(":/icons/calculator.svg"));
    ui->resultLabel->setContentsMargins(5, 0, 5, 0);
    ui->financeResultLabel->setContentsMargins(5, 0, 5, 0);

    for (auto button : ui->keyPanel->findChildren<QPushButton *>()) {
        button->setMinimumHeight(38);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
    for (int row = 0; row < 5; ++row) ui->keyLayout->setRowStretch(row, 1);
    for (int column = 0; column < 4; ++column)
        ui->keyLayout->setColumnStretch(column, 1);
    ui->mainLayout->setStretch(2, 1);

    // 普通点击和拖拽输入共用同一套追加逻辑。
    for (auto button : ui->keyPanel->findChildren<DraggableButton *>()) {
        connect(button, &QPushButton::clicked, this, [this, button] {
            appendText(button->property("value").toString());
        });
        connect(button, &DraggableButton::dropped, this,
                [this](const QString &text, const QPoint &globalPos) {
                    if (ui->expressionEdit->rect().contains(
                          ui->expressionEdit->mapFromGlobal(globalPos)))
                        appendText(text);
                });
    }

    connect(ui->equalButton, &QPushButton::clicked, this,
            &MainWindow::calculate);
    connect(ui->clearButton, &QPushButton::clicked, this, [this] {
        ui->expressionEdit->clear();
        ui->resultLabel->setText("0");
    });
    connect(ui->backButton, &QPushButton::clicked, ui->expressionEdit,
            &QLineEdit::backspace);
    connect(ui->financeButton, &QPushButton::clicked, this,
            &MainWindow::openFinancialDialog);
    connect(ui->principalSpin,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &MainWindow::updateFinanceResult);
    updateFinanceResult();

    // 不允许窗口小于布局所需尺寸，避免按钮与财务区域重叠
    ui->mainLayout->activate();
    setMinimumSize(sizeHint());
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::appendText(const QString &text) {
    ui->expressionEdit->insert(text);
}

void MainWindow::calculate() {
    QString error;
    double answer = 0;
    if (!model.evaluate(ui->expressionEdit->text(), answer, error)) {
        QMessageBox::warning(this, "计算错误", error);
        return;
    }
    ui->resultLabel->setText(QString::number(answer, 'g', 12));
}

void MainWindow::openFinancialDialog() {
    FinancialDialog dialog(rate, years, compound, this);
    // 对话框发出参数信号，主窗口接收并刷新财务结果。
    connect(&dialog, &FinancialDialog::parametersChanged, this,
            [this](double newRate, int newYears, bool useCompound) {
                rate = newRate;
                years = newYears;
                compound = useCompound;
                updateFinanceResult();
            });
    dialog.exec();
}

void MainWindow::updateFinanceResult() {
    double principal = ui->principalSpin->value();
    double total = compound ? model.compoundInterest(principal, rate, years)
                            : model.simpleInterest(principal, rate, years);
    const QLocale locale(QLocale::Chinese, QLocale::China);
    ui->financeResultLabel->setText(locale.toString(total, 'f', 2) + " 元");
    ui->parameterLabel->setText(QString("%1，年利率 %2%，期限 %3 年")
                                  .arg(compound ? "复利" : "单利")
                                  .arg(locale.toString(rate, 'f', 2))
                                  .arg(years));
}
