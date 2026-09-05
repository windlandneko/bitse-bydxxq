import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Button {
  id: control
  property string tone: 'primary'
  property string leadingIcon: ''
  property string trailingIcon: ''
  implicitHeight: Theme.touchSize
  implicitWidth: Math.max(120, contentItem.implicitWidth + leftPadding + rightPadding)
  horizontalPadding: 16
  verticalPadding: 8
  font.pixelSize: Theme.bodySize
  font.weight: Font.Medium
  focusPolicy: Qt.StrongFocus
  Accessible.name: text
  Accessible.onPressAction: clicked()
  contentItem: RowLayout {
    spacing: Theme.space
    AppIcon {
      name: control.leadingIcon
      visible: !!control.leadingIcon
      Layout.preferredWidth: 24
      Layout.preferredHeight: 24
    }
    AppText {
      Layout.fillWidth: true
      text: control.text
      font: control.font
      color: !control.enabled ? Theme.disabledText : control.tone === 'primary' ? 'white' : control.tone === 'danger' ? Theme.danger : Theme.primary
      horizontalAlignment: Text.AlignHCenter
      verticalAlignment: Text.AlignVCenter
      elide: Text.ElideRight
    }
    AppIcon {
      name: control.trailingIcon
      visible: !!control.trailingIcon
      Layout.preferredWidth: 24
      Layout.preferredHeight: 24
    }
  }
  background: Rectangle {
    radius: Theme.cardRadius
    color: !control.enabled ? Theme.disabled : control.tone === 'primary' ? (control.down ? Theme.primaryPressed : Theme.primary) : control.tone === 'danger' ? (control.down ? '#f3d8cf' : Theme.dangerLight) : control.down ? '#d4e4d5' : control.tone === 'quiet' ? (control.hovered ? Theme.primaryLight : 'transparent') : Theme.primaryLight
    border.color: control.tone === 'primary' ? Theme.accent : Theme.primary
    border.width: control.visualFocus ? 2 : 0
  }
}
