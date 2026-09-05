import QtQuick 2.15
import QtQuick.Controls 2.15

Button {
  id: control
  property string iconName: ''
  property string label: ''
  implicitWidth: Theme.touchSize
  implicitHeight: Theme.touchSize
  padding: 12
  focusPolicy: Qt.StrongFocus
  Accessible.name: label
  Accessible.onPressAction: clicked()
  contentItem: AppIcon {
    name: control.iconName
    opacity: control.enabled ? 1 : 0.4
  }
  background: Rectangle {
    radius: Theme.heroRadius
    color: !control.enabled ? 'transparent' : control.down ? '#d4e4d5' : control.hovered ? Theme.primaryLight : 'transparent'
    border.width: control.visualFocus ? 2 : 0
    border.color: Theme.primary
  }
}
