#ifndef CALCULATORMODEL_H
#define CALCULATORMODEL_H

#include <QString>

class CalculatorModel {
  public:
    bool evaluate(const QString &expression, double &result,
                  QString &error) const;
    double simpleInterest(double principal, double rate, int years) const;
    double compoundInterest(double principal, double rate, int years) const;
};

#endif
