#pragma once

#include <QMainWindow>
#include <memory>

class AdminMainWindow final : public QMainWindow {
public:
  explicit AdminMainWindow(QWidget *parent = nullptr);
  ~AdminMainWindow() override;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
