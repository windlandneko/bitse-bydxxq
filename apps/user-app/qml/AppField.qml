import QtQuick 2.15
import QtQuick.Controls 2.15

TextField {
  id: control
  implicitHeight: 54
  font.pixelSize: 15
  color: Theme.ink
  placeholderTextColor: '#a0aaa2'
  leftPadding: 16
  rightPadding: 16
  selectByMouse: true
  selectionColor: Theme.primaryLight
  selectedTextColor: Theme.ink
  background: Rectangle {
    color: Theme.card
    radius: 14
    border.color: control.activeFocus ? Theme.primary : Theme.border
    border.width: control.activeFocus ? 2 : 1
  }
}
