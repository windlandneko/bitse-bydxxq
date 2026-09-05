import QtQuick 2.15

Column {
  property string title: '这里还没有内容'
  property string description: ''
  spacing: Theme.cardPadding
  Rectangle {
    anchors.horizontalCenter: parent.horizontalCenter
    width: 64
    height: 64
    radius: Theme.heroRadius
    color: Theme.primaryLight
    AppIcon {
      anchors.centerIn: parent
      name: 'list'
      opacity: 0.65
    }
  }
  AppText {
    width: parent.width
    text: parent.title
    font.pixelSize: Theme.titleSize
    font.weight: Font.DemiBold
    horizontalAlignment: Text.AlignHCenter
    wrapMode: Text.WordWrap
  }
  AppText {
    width: parent.width
    visible: !!parent.description
    text: parent.description
    color: Theme.muted
    wrapMode: Text.WordWrap
    lineHeight: 1.5
    horizontalAlignment: Text.AlignHCenter
  }
}
