import QtQuick 2.15
import QtQuick.Controls 2.15

TextField {
  id: control
  implicitHeight: 56
  font.pixelSize: Theme.bodyLargeSize
  color: enabled ? Theme.ink : Theme.disabledText
  placeholderTextColor: Theme.muted
  leftPadding: Theme.cardPadding
  rightPadding: Theme.cardPadding
  selectByMouse: true
  selectionColor: Theme.primaryLight
  selectedTextColor: Theme.ink
  Accessible.name: placeholderText
  background: Rectangle {
    color: control.enabled ? Theme.card : Theme.disabled
    radius: Theme.cardRadius
    border.color: control.activeFocus ? Theme.primary : Theme.border
    border.width: control.activeFocus ? 2 : 1
  }
}
