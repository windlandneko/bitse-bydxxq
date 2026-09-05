import QtQuick 2.15

Item {
  property string label: ''
  property string value: ''
  property color valueColor: Theme.ink
  implicitHeight: Math.max(name.implicitHeight, content.implicitHeight)
  AppText {
    id: name
    width: parent.width * 0.3
    text: parent.label
    font.pixelSize: 13
    color: Theme.muted
  }
  AppText {
    id: content
    anchors.right: parent.right
    width: parent.width * 0.68
    text: parent.value
    font.pixelSize: 13
    color: parent.valueColor
    horizontalAlignment: Text.AlignRight
    wrapMode: Text.WrapAnywhere
  }
}
