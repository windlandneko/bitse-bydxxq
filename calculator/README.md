# 计算器

课程作业项目，使用 C++ 与 Qt Widgets 编写。支持四则运算、单利/复利、参数设置对话框，以及将数字或运算符按钮拖入输入框。

## 构建

```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-tools-dev-tools

cmake --preset debug
cmake --build build/debug -j"$(nproc)"
./build/debug/calculator/calculator
```

也可以在 Qt Creator 中打开 `calculator.pro` 或仓库根目录的 `CMakeLists.txt`。

## 代码结构

- `mainwindow.ui`：主窗口与布局
- `financialdialog.ui`：财务参数设置对话框
- `calculatormodel.*`：四则运算与利息计算逻辑
- `draggablebutton.*`：鼠标事件重写与拖拽阴影
- `resources.qrc`：SVG 图标
