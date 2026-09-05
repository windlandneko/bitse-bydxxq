import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
  id: card
  property var stationData: ({})
  property bool highlighted: false
  implicitHeight: info.height + 38
  radius: 20
  color: 'white'
  border.color: highlighted ? '#cbdebd' : Theme.border
  border.width: highlighted ? 2 : 1
  objectName: 'stationCard_' + (stationData.id || 0)
  MouseArea {
    anchors.fill: parent
    onClicked: mobile.openStation(Number(card.stationData.id))
  }
  Column {
    id: info
    x: 18
    y: 18
    width: parent.width - 36
    spacing: 12
    Row {
      width: parent.width
      spacing: 10
      Rectangle {
        width: 40
        height: 40
        radius: 13
        color: card.highlighted ? Theme.accent : Theme.primaryLight
        Image {
          anchors.centerIn: parent
          width: 21
          height: 21
          source: 'qrc:/icons/zap.svg'
        }
      }
      Column {
        width: parent.width - 50
        spacing: 6
        AppText {
          width: parent.width
          text: card.stationData.name || ''
          font.pixelSize: 16
          font.weight: Font.Bold
          elide: Text.ElideRight
        }
        AppText {
          width: parent.width
          text: card.stationData.address || ''
          color: Theme.muted
          font.pixelSize: 11
          elide: Text.ElideRight
        }
      }
    }
    Row {
      spacing: 6
      visible: card.highlighted
      Badge {
        text: '低拥堵推荐'
        fill: '#eff5dd'
        textColor: '#557326'
      }
      Badge {
        text: '1 小时后预计空闲 ' + Number(card.stationData.predictedAvailableChargers || 0) + ' 桩'
        fill: '#f4f6f1'
        textColor: Theme.muted
      }
    }
    Rectangle {
      width: parent.width
      height: 1
      color: '#f0f2ed'
    }
    Item {
      width: parent.width
      height: 34
      Row {
        anchors.verticalCenter: parent.verticalCenter
        spacing: 3
        AppText {
          text: '¥'
          font.pixelSize: 12
          y: 8
          color: Theme.primary
        }
        AppText {
          text: (Number(card.stationData.priceCents || 0) / 100).toFixed(2)
          font.pixelSize: 26
          font.weight: Font.Bold
          color: Theme.primary
        }
        AppText {
          text: '/度'
          font.pixelSize: 11
          color: Theme.muted
          y: 13
        }
      }
      Row {
        anchors.right: distanceButton.left
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2
        AppText {
          text: String(card.stationData.idleChargers || 0)
          color: Theme.primary
          font.pixelSize: 16
          font.weight: Font.Bold
        }
        AppText {
          text: '/' + (card.stationData.totalChargers || 0) + ' 空闲'
          color: Theme.muted
          font.pixelSize: 11
          y: 4
        }
      }
      ActionButton {
        id: distanceButton
        objectName: 'navigateStation_' + (card.stationData.id || 0)
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        width: 88
        height: 34
        font.pixelSize: 11
        horizontalPadding: 8
        text: Number(card.stationData.distanceKm || 0).toFixed(1) + ' km ↗'
        tone: 'secondary'
        onClicked: mobile.openNavigation(card.stationData)
      }
    }
  }
}
