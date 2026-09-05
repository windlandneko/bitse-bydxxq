import QtQuick 2.15

Rectangle {
  property alias text: label.text
  property color textColor: Theme.primary
  property color fill: Theme.primaryLight
  implicitHeight: 25
  implicitWidth: label.implicitWidth + 16
  radius: 7
  color: fill
  AppText {
    id: label
    anchors.centerIn: parent
    font.pixelSize: 11
    font.weight: Font.DemiBold
    color: parent.textColor
  }
}
