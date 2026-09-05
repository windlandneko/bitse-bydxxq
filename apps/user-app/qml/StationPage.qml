import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Flickable {
  objectName: 'stationPage'
  contentWidth: width
  contentHeight: content.height + Theme.pagePadding * 2
  boundsBehavior: Flickable.StopAtBounds
  clip: true
  ScrollBar.vertical: ScrollBar {
    policy: ScrollBar.AsNeeded
  }
  Column {
    id: content
    x: Theme.pagePadding
    y: Theme.pagePadding
    width: parent.width - Theme.pagePadding * 2
    spacing: Theme.cardPadding
    Rectangle {
      width: parent.width
      height: header.height + Theme.cardPadding * 2
      radius: Theme.heroRadius
      color: Theme.primary
      Column {
        id: header
        x: Theme.cardPadding
        y: Theme.cardPadding
        width: parent.width - Theme.cardPadding * 2
        spacing: Theme.cardPadding
        Badge {
          text: mobile.station.region || '城市补能'
          maximumWidth: parent.width
          fill: '#42785c'
          textColor: 'white'
        }
        Column {
          width: parent.width
          spacing: Theme.space
          AppText {
            width: parent.width
            text: mobile.station.name || ''
            font.pixelSize: Theme.titleSize
            font.weight: Font.DemiBold
            color: 'white'
            wrapMode: Text.WordWrap
          }
          AppText {
            width: parent.width
            text: mobile.station.address || ''
            color: '#d2e2d5'
            font.pixelSize: Theme.bodySize
            wrapMode: Text.WordWrap
            lineHeight: 1.4
          }
        }
        RowLayout {
          width: parent.width
          spacing: Theme.cardPadding
          Column {
            Layout.fillWidth: true
            spacing: Theme.microSpace
            MoneyText {
              cents: Number(mobile.station.priceCents || 0)
              valueColor: Theme.accent
            }
            AppText {
              text: '电价（元/度）'
              color: '#d2e2d5'
              font.pixelSize: Theme.labelSize
            }
          }
          Column {
            Layout.fillWidth: true
            spacing: Theme.microSpace
            AppText {
              text: (mobile.station.idleChargers || 0) + ' / ' + (mobile.station.totalChargers || 0)
              color: 'white'
              font.pixelSize: Theme.headlineSize
              font.weight: Font.DemiBold
            }
            AppText {
              text: '空闲电桩 / 全部'
              color: '#d2e2d5'
              font.pixelSize: Theme.labelSize
            }
          }
        }
        ActionButton {
          objectName: 'stationNavigationButton'
          width: parent.width
          tone: 'secondary'
          text: '直线 ' + Number(mobile.station.distanceKm || 0).toFixed(1) + ' km · 导航'
          trailingIcon: 'navigation'
          onClicked: mobile.openNavigation(mobile.station)
        }
      }
    }
    Rectangle {
      width: parent.width
      height: forecastNote.implicitHeight + Theme.cardPadding * 2
      visible: !!mobile.station.forecastAt
      radius: Theme.cardRadius
      color: '#edf1df'
      AppText {
        id: forecastNote
        x: Theme.cardPadding
        y: Theme.cardPadding
        width: parent.width - Theme.cardPadding * 2
        text: '1 小时后预计空闲 ' + Number(mobile.station.predictedAvailableChargers || 0) + ' 桩'
        font.pixelSize: Theme.bodySize
        color: '#586e35'
        wrapMode: Text.WordWrap
        lineHeight: 1.5
      }
    }
    AppText {
      text: '选择充电桩'
      font.pixelSize: Theme.bodyLargeSize
      font.weight: Font.Medium
    }
    Repeater {
      model: mobile.chargers
      delegate: Rectangle {
        id: chargerCard
        required property var modelData
        objectName: 'chargerCard_' + modelData.id
        width: content.width
        height: chargerContent.height + Theme.cardPadding * 2
        radius: Theme.cardRadius
        color: Theme.card
        border.color: modelData.status === 'idle' ? '#d2e0ce' : Theme.border
        Column {
          id: chargerContent
          x: Theme.cardPadding
          y: Theme.cardPadding
          width: parent.width - Theme.cardPadding * 2
          spacing: Theme.cardPadding
          RowLayout {
            width: parent.width
            spacing: Theme.controlGap
            Rectangle {
              Layout.preferredWidth: 48
              Layout.preferredHeight: 48
              radius: Theme.cardRadius
              color: Theme.primaryLight
              AppIcon {
                anchors.centerIn: parent
                name: 'zap'
              }
            }
            Column {
              Layout.fillWidth: true
              spacing: Theme.microSpace
              AppText {
                text: chargerCard.modelData.code
                font.pixelSize: Theme.bodyLargeSize
                font.weight: Font.Medium
              }
              AppText {
                text: (chargerCard.modelData.type === 'dc' ? '直流快充' : '交流慢充') + ' · ' + chargerCard.modelData.powerKw + ' kW'
                color: Theme.muted
                font.pixelSize: Theme.labelSize
              }
            }
          }
          RowLayout {
            width: parent.width
            Badge {
              text: mobile.statusLabel(chargerCard.modelData.status)
              textColor: chargerCard.modelData.status === 'idle' ? Theme.primary : chargerCard.modelData.status === 'fault' ? Theme.danger : Theme.muted
              fill: chargerCard.modelData.status === 'idle' ? Theme.primaryLight : chargerCard.modelData.status === 'fault' ? Theme.dangerLight : '#f1f2ef'
            }
            Item {
              Layout.fillWidth: true
            }
            ActionButton {
              objectName: 'reserveCharger_' + chargerCard.modelData.id
              Layout.preferredWidth: 120
              text: chargerCard.modelData.status === 'idle' ? '预约充电' : '暂不可用'
              enabled: chargerCard.modelData.status === 'idle' && !mobile.busy
              onClicked: mobile.reserve(Number(chargerCard.modelData.id))
            }
          }
        }
      }
    }
    EmptyState {
      width: parent.width
      visible: mobile.chargers.length === 0
      title: '暂时没有电桩信息'
      description: '刷新后重试。'
    }
    AppText {
      width: parent.width
      text: '预约保留 15 分钟，按充电量计费。'
      color: Theme.muted
      font.pixelSize: Theme.labelSize
      wrapMode: Text.WordWrap
      lineHeight: 1.4
    }
  }
}
