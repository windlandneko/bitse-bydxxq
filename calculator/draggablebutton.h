#ifndef DRAGGABLEBUTTON_H
#define DRAGGABLEBUTTON_H

#include <QPushButton>

class DraggableButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(QString value READ value WRITE setValue)
  public:
    explicit DraggableButton(QWidget *parent = nullptr);
    QString value() const { return buttonValue; }
    void setValue(const QString &text) { buttonValue = text; }

  signals:
    void dropped(const QString &text, const QPoint &globalPos);

  protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

  private:
    QPoint pressPos;
    QPushButton *shadow = nullptr;
    bool dragging = false;
    QString buttonValue;
};

#endif
