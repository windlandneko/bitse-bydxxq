import QtQuick 2.15
import QtQuick.Layouts 1.15

RowLayout {
  id: money
  required property real cents
  property string suffix: ''
  property color valueColor: Theme.primary
  property int valueSize: Theme.headlineSize
  spacing: Theme.microSpace
  AppText {
    text: '¥'
    font.pixelSize: Theme.bodySize
    color: money.valueColor
    Layout.alignment: Qt.AlignBaseline
  }
  AppText {
    text: (money.cents / 100).toFixed(2)
    font.pixelSize: money.valueSize
    font.weight: Font.DemiBold
    color: money.valueColor
    Layout.alignment: Qt.AlignBaseline
  }
  AppText {
    visible: !!money.suffix
    text: money.suffix
    font.pixelSize: Theme.labelSize
    color: money.valueColor
    Layout.alignment: Qt.AlignBaseline
  }
}
