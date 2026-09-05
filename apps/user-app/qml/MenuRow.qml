import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Button {
  id: control
  property string title: ''
  property string description: ''
  property string iconName: ''
  implicitHeight: 80
  padding: Theme.cardPadding
  Accessible.name: title + (description ? '，' + description : '')
  background: Rectangle {
    radius: Theme.cardRadius
    color: control.down ? Theme.primaryLight : Theme.card
    border.width: control.visualFocus ? 2 : 0
    border.color: Theme.primary
  }
  contentItem: RowLayout {
    spacing: Theme.cardPadding
    AppIcon {
      name: control.iconName
      Layout.preferredWidth: 24
      Layout.preferredHeight: 24
    }
    Column {
      Layout.fillWidth: true
      spacing: Theme.microSpace
      AppText {
        width: parent.width
        text: control.title
        font.pixelSize: Theme.bodyLargeSize
        font.weight: Font.Medium
        elide: Text.ElideRight
      }
      AppText {
        width: parent.width
        text: control.description
        visible: !!control.description
        font.pixelSize: Theme.labelSize
        color: Theme.muted
        elide: Text.ElideRight
      }
    }
    AppIcon {
      name: 'chevron-right'
      Layout.preferredWidth: 24
      Layout.preferredHeight: 24
    }
  }
}
