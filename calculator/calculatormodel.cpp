#include "calculatormodel.h"

#include <QStack>
#include <QtMath>

namespace {
int priority(QChar op) { return op == '+' || op == '-' ? 1 : 2; }

bool apply(QStack<double> &numbers, QChar op, QString &error) {
    if (numbers.size() < 2) {
        error = "表达式格式错误";
        return false;
    }
    double b = numbers.pop(), a = numbers.pop();
    if (op == '/' && qFuzzyIsNull(b)) {
        error = "除数不能为零";
        return false;
    }
    if (op == '+') numbers.push(a + b);
    if (op == '-') numbers.push(a - b);
    if (op == '*') numbers.push(a * b);
    if (op == '/') numbers.push(a / b);
    return true;
}
} // namespace

bool CalculatorModel::evaluate(const QString &source, double &result,
                               QString &error) const {
    QString s = source;
    s.remove(' ');
    if (s.isEmpty()) {
        error = "请输入表达式";
        return false;
    }
    QStack<double> numbers;
    QStack<QChar> operators;
    // 双栈求值保留乘除优先级；needNumber 同时用于识别一元负号
    bool needNumber = true;
    for (int i = 0; i < s.size();) {
        if (s[i].isDigit() || s[i] == '.' || (s[i] == '-' && needNumber)) {
            int start = i++;
            while (i < s.size() && (s[i].isDigit() || s[i] == '.')) ++i;
            bool ok = false;
            double value = s.mid(start, i - start).toDouble(&ok);
            if (!ok) {
                error = "数字格式错误";
                return false;
            }
            numbers.push(value);
            needNumber = false;
        } else if (QString("+-*/").contains(s[i]) && !needNumber) {
            while (!operators.isEmpty()
                   && priority(operators.top()) >= priority(s[i]))
                if (!apply(numbers, operators.pop(), error)) return false;
            operators.push(s[i++]);
            needNumber = true;
        } else {
            error = "表达式含有无效字符或连续运算符";
            return false;
        }
    }
    if (needNumber) {
        error = "表达式不能以运算符结尾";
        return false;
    }
    while (!operators.isEmpty())
        if (!apply(numbers, operators.pop(), error)) return false;
    result = numbers.top();
    return true;
}

double CalculatorModel::simpleInterest(double principal, double rate,
                                       int years) const {
    return principal * (1 + rate / 100.0 * years);
}

double CalculatorModel::compoundInterest(double principal, double rate,
                                         int years) const {
    return principal * qPow(1 + rate / 100.0, years);
}
