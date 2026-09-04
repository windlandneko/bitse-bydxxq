#include "draggablebutton.h"

#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QMouseEvent>

DraggableButton::DraggableButton(QWidget *parent) : QPushButton(parent) {
    setCursor(Qt::OpenHandCursor);
}

void DraggableButton::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        // 保留按下位置，用系统拖拽阈值区分单击与拖拽，避免轻微抖动误触。
        pressPos = event->position().toPoint();
        dragging = false;
    }
    QPushButton::mousePressEvent(event);
}

void DraggableButton::mouseMoveEvent(QMouseEvent *event) {
    if (!(event->buttons() & Qt::LeftButton)) return;
    if (!dragging
        && (event->position().toPoint() - pressPos).manhattanLength()
             < QApplication::startDragDistance())
        return;
    if (!shadow) {
        // 阴影挂在顶层窗口上并忽略鼠标事件，保证释放事件仍由原按钮接收。
        shadow = new QPushButton(text(), window());
        shadow->setObjectName("dragShadow");
        shadow->setAttribute(Qt::WA_TransparentForMouseEvents);
        shadow->setFixedSize(size());
        auto effect = new QGraphicsOpacityEffect(shadow);
        effect->setOpacity(0.75);
        shadow->setGraphicsEffect(effect);
        shadow->show();
        shadow->raise();
    }
    dragging = true;
    const QPoint topLeft = event->globalPosition().toPoint() - pressPos;
    shadow->move(shadow->parentWidget()->mapFromGlobal(topLeft));
}

void DraggableButton::mouseReleaseEvent(QMouseEvent *event) {
    if (dragging) {
        // 主窗口使用全局坐标判断是否落入输入区，避免控件坐标系耦合。
        emit dropped(value(), event->globalPosition().toPoint());
        shadow->hide();
        delete shadow;
        shadow = nullptr;
        dragging = false;
        setDown(false);
        return;
    }
    QPushButton::mouseReleaseEvent(event);
}
