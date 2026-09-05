import QtQuick 2.15

Rectangle {
  property alias text: label.text
  property color textColor: Theme.primary
  property color fill: Theme.primaryLight
  property real maximumWidth: Number.POSITIVE_INFINITY
  implicitHeight: 28
  implicitWidth: Math.min(label.implicitWidth + Theme.space * 2, maximumWidth)
  radius: 8
  color: fill
  AppText {
    id: label
    anchors.centerIn: parent
    width: Math.max(0, parent.width - Theme.space * 2)
    horizontalAlignment: Text.AlignHCenter
    elide: Text.ElideRight
    font.pixelSize: Theme.labelSize
    font.weight: Font.Medium
    color: parent.textColor
  }
}
