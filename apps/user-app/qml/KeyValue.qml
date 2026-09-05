import QtQuick 2.15
import QtQuick.Layouts 1.15

RowLayout {
  property string label: ''
  property string value: ''
  property color valueColor: Theme.ink
  spacing: Theme.space
  AppText {
    Layout.preferredWidth: 72
    Layout.alignment: Qt.AlignTop
    text: parent.label
    color: Theme.muted
    font.pixelSize: Theme.bodySize
  }
  AppText {
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop
    text: parent.value
    color: parent.valueColor
    font.pixelSize: Theme.bodySize
    horizontalAlignment: Text.AlignRight
    wrapMode: Text.WrapAnywhere
  }
}
