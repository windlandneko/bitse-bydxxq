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
      color: {
        if (!control.enabled)
          return Theme.disabledText
        return control.tone === 'primary' ? 'white' : Theme.primary
      }
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
    color: {
      if (!control.enabled)
        return Theme.disabled
      if (control.tone === 'primary')
        return control.down ? Theme.primaryPressed : Theme.primary
      if (control.down)
        return '#d4e4d5'
      if (control.tone === 'quiet' && !control.hovered)
        return 'transparent'
      return Theme.primaryLight
    }
    border.color: control.tone === 'primary' ? Theme.accent : Theme.primary
    border.width: control.visualFocus ? 2 : 0
  }
}
