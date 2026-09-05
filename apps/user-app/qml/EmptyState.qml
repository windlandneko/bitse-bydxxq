import QtQuick 2.15

Column {
  property string title: '这里还没有内容'
  property string description: ''
  spacing: 14
  Rectangle {
    anchors.horizontalCenter: parent.horizontalCenter
    width: 78
    height: 78
    radius: 26
    color: Theme.primaryLight
    Image {
      anchors.centerIn: parent
      source: 'qrc:/icons/zap.svg'
      width: 32
      height: 32
      opacity: 0.5
    }
  }
  AppText {
    width: parent.width
    text: parent.title
    font.pixelSize: 18
    font.weight: Font.DemiBold
    horizontalAlignment: Text.AlignHCenter
  }
  AppText {
    width: parent.width
    text: parent.description
    color: Theme.muted
    wrapMode: Text.WordWrap
    lineHeight: 1.5
    horizontalAlignment: Text.AlignHCenter
  }
}
