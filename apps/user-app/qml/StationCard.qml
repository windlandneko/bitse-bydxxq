import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Button {
  id: card
  property var stationData: ({})
  objectName: 'stationCard_' + (stationData.id || 0)
  padding: Theme.cardPadding
  implicitHeight: info.implicitHeight + topPadding + bottomPadding
  Accessible.name: (stationData.name || '电站') + '，空闲 ' + (stationData.idleChargers || 0) + ' 个充电桩，查看详情'
  onClicked: mobile.openStation(Number(stationData.id))
  background: Rectangle {
    radius: Theme.cardRadius
    color: card.down ? '#f0f5ed' : Theme.card
    border.color: card.visualFocus ? Theme.primary : card.highlighted ? '#bed2a9' : Theme.border
    border.width: card.visualFocus ? 2 : 1
  }
  contentItem: Column {
    id: info
    spacing: Theme.cardPadding
    RowLayout {
      width: parent.width
      spacing: Theme.controlGap
      Rectangle {
        Layout.preferredWidth: 48
        Layout.preferredHeight: 48
        radius: Theme.cardRadius
        color: card.highlighted ? Theme.accent : Theme.primaryLight
        AppIcon {
          anchors.centerIn: parent
          name: 'zap'
        }
      }
      Column {
        Layout.fillWidth: true
        spacing: Theme.microSpace
        AppText {
          width: parent.width
          text: card.stationData.name || ''
          font.pixelSize: Theme.bodyLargeSize
          font.weight: Font.Medium
          elide: Text.ElideRight
        }
        AppText {
          width: parent.width
          text: card.stationData.address || ''
          font.pixelSize: Theme.labelSize
          color: Theme.muted
          elide: Text.ElideRight
        }
      }
    }
    Flow {
      width: parent.width
      spacing: Theme.space
      visible: card.highlighted
      Badge {
        text: '低拥堵推荐'
        fill: '#edf3dd'
        textColor: '#526e27'
      }
      Badge {
        text: '1 小时后预计空闲 ' + Number(card.stationData.predictedAvailableChargers || 0) + ' 桩'
        fill: '#f0f3eb'
        textColor: Theme.muted
      }
    }
    RowLayout {
      width: parent.width
      spacing: Theme.cardPadding
      Column {
        Layout.fillWidth: true
        spacing: Theme.microSpace
        MoneyText {
          objectName: 'stationPrice'
          cents: Number(card.stationData.priceCents || 0)
          suffix: '/度'
        }
        AppText {
          text: '空闲 ' + (card.stationData.idleChargers || 0) + ' / ' + (card.stationData.totalChargers || 0) + ' 桩'
          font.pixelSize: Theme.labelSize
          color: Theme.muted
        }
      }
      ActionButton {
        objectName: 'navigateStation_' + (card.stationData.id || 0)
        Layout.preferredWidth: 112
        text: Number(card.stationData.distanceKm || 0).toFixed(1) + ' km'
        trailingIcon: 'navigation'
        horizontalPadding: Theme.space
        tone: 'secondary'
        Accessible.name: '导航到' + (card.stationData.name || '') + '，距离' + Number(card.stationData.distanceKm || 0).toFixed(1) + '公里'
        onClicked: mobile.openNavigation(card.stationData)
      }
    }
  }
}
