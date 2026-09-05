import QtQuick 2.15
import QtQuick.Controls 2.15

Button {
  id: control
  property string tone: 'primary'
  implicitHeight: 52
  implicitWidth: 140
  horizontalPadding: 18
  font.pixelSize: 15
  font.weight: Font.DemiBold
  opacity: enabled ? 1 : 0.45
  contentItem: AppText {
    text: control.text
    font: control.font
    color: control.tone === 'primary' ? 'white' : control.tone === 'danger' ? Theme.danger : Theme.primary
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
    elide: Text.ElideRight
  }
  background: Rectangle {
    radius: 15
    color: control.tone === 'primary' ? (control.down ? '#174a33' : Theme.primary) : control.tone === 'danger' ? Theme.dangerLight : control.tone === 'quiet' ? 'transparent' : Theme.primaryLight
    border.color: control.visualFocus ? Theme.primary : 'transparent'
    border.width: 2
  }
}
