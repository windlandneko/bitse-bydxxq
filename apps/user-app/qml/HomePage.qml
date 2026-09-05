import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Flickable {
  id: screen
  objectName: 'homePage'
  contentWidth: width
  contentHeight: content.height + Theme.pagePadding * 2
  clip: true
  boundsBehavior: Flickable.StopAtBounds
  property var suggestion: {
    for (var i = 0; i < mobile.stations.length; i++)
      if (mobile.stations[i].recommended && Number(mobile.stations[i].predictedAvailableChargers) > 0)
        return mobile.stations[i]
    return null
  }
  ScrollBar.vertical: ScrollBar {
    policy: ScrollBar.AsNeeded
  }
  Column {
    id: content
    x: Theme.pagePadding
    y: Theme.pagePadding
    width: parent.width - Theme.pagePadding * 2
    spacing: Theme.cardPadding
    Button {
      id: locationButton
      objectName: 'chooseLocationButton'
      width: parent.width
      height: 56
      padding: Theme.cardPadding
      Accessible.name: '当前位置：' + mobile.locationName + '，选择位置'
      onClicked: mobile.navigate('location')
      background: Rectangle {
        radius: Theme.cardRadius
        color: locationButton.down ? '#d3e3d6' : Theme.primaryLight
        border.color: Theme.primary
        border.width: locationButton.visualFocus ? 2 : 0
      }
      contentItem: RowLayout {
        spacing: Theme.controlGap
        AppIcon {
          name: 'map-pin'
          Layout.preferredWidth: 24
          Layout.preferredHeight: 24
        }
        AppText {
          Layout.fillWidth: true
          text: mobile.locationName
          color: Theme.primary
          font.pixelSize: Theme.bodySize
          elide: Text.ElideRight
        }
        AppIcon {
          name: 'chevron-down'
          Layout.preferredWidth: 24
          Layout.preferredHeight: 24
        }
      }
    }
    AppField {
      id: search
      objectName: 'stationSearchInput'
      width: parent.width
      placeholderText: '搜索电站名称 / 地址'
      text: mobile.query
      onTextEdited: {
        mobile.query = text
        searchDebounce.restart()
      }
      onAccepted: {
        searchDebounce.stop()
        mobile.refreshStations()
        focus = false
      }
      Timer {
        id: searchDebounce
        interval: 350
        onTriggered: mobile.refreshStations()
      }
    }
    Button {
      id: activeBanner
      objectName: 'activeOrderBanner'
      width: parent.width
      implicitHeight: activeInfo.implicitHeight + Theme.cardPadding * 2
      visible: Number(mobile.activeOrder.id || 0) > 0
      padding: Theme.cardPadding
      Accessible.name: '处理' + mobile.statusLabel(mobile.activeOrder.status || '') + '订单'
      onClicked: mobile.openActiveOrder()
      background: Rectangle {
        radius: Theme.cardRadius
        color: activeBanner.down ? '#e7dfbd' : '#f0eddc'
        border.color: '#e0d4a8'
        border.width: activeBanner.visualFocus ? 2 : 1
      }
      contentItem: RowLayout {
        spacing: Theme.space
        Column {
          id: activeInfo
          Layout.fillWidth: true
          spacing: Theme.microSpace
          AppText {
            text: '待处理：' + mobile.statusLabel(mobile.activeOrder.status || '') + '订单'
            font.weight: Font.Medium
            color: '#796230'
          }
          AppText {
            text: mobile.activeOrder.stationName || ''
            width: parent.width
            elide: Text.ElideRight
            font.pixelSize: Theme.labelSize
            color: '#796230'
          }
        }
        AppIcon {
          name: 'chevron-right'
          Layout.preferredWidth: 24
          Layout.preferredHeight: 24
        }
      }
    }
    Rectangle {
      width: parent.width
      height: 160
      radius: Theme.heroRadius
      color: Theme.primary
      RowLayout {
        anchors.fill: parent
        anchors.margins: Theme.cardPadding
        spacing: Theme.space
        Column {
          Layout.fillWidth: true
          spacing: Theme.space
          AppText {
            width: parent.width
            text: '好电站，近一点'
            color: 'white'
            font.pixelSize: Theme.titleSize
            font.weight: Font.DemiBold
            wrapMode: Text.WordWrap
          }
          AppText {
            text: '附近 ' + mobile.stations.length + ' 座电站'
            color: '#d2e2d5'
            font.pixelSize: Theme.labelSize
          }
        }
        HeroIllustration {
          Layout.preferredWidth: screen.width < 400 ? 128 : 152
          Layout.fillHeight: true
        }
      }
    }
    Column {
      width: parent.width
      spacing: Theme.cardPadding
      visible: screen.suggestion !== null
      RowLayout {
        width: parent.width
        AppText {
          Layout.fillWidth: true
          text: '为你推荐'
          font.pixelSize: Theme.bodyLargeSize
          font.weight: Font.Medium
        }
        AppText {
          text: '根据空闲预测'
          font.pixelSize: Theme.labelSize
          color: Theme.muted
        }
      }
      StationCard {
        width: parent.width
        stationData: screen.suggestion || ({})
        highlighted: true
      }
    }
    Column {
      width: parent.width
      spacing: Theme.space
      RowLayout {
        width: parent.width
        AppText {
          Layout.fillWidth: true
          text: '附近电站'
          font.pixelSize: Theme.bodyLargeSize
          font.weight: Font.Medium
        }
        ActionButton {
          objectName: 'fastOnlyButton'
          Layout.preferredWidth: 88
          text: '仅快充'
          horizontalPadding: Theme.space
          tone: mobile.fastOnly ? 'primary' : 'secondary'
          onClicked: mobile.fastOnly = !mobile.fastOnly
        }
      }
      Row {
        width: parent.width
        spacing: Theme.space
        Repeater {
          model: [{
              "key": 'distance',
              "text": '距离优先'
            }, {
              "key": 'price',
              "text": '价格最低'
            }, {
              "key": 'idle',
              "text": '空闲最多'
            }]
          delegate: ActionButton {
            required property var modelData
            objectName: 'sort_' + modelData.key
            width: (content.width - Theme.space * 2) / 3
            text: modelData.text
            horizontalPadding: Theme.space
            tone: mobile.sort === modelData.key ? 'primary' : 'secondary'
            onClicked: mobile.sort = modelData.key
          }
        }
      }
    }
    Repeater {
      model: mobile.stations
      delegate: StationCard {
        required property var modelData
        width: content.width
        stationData: modelData
      }
    }
    EmptyState {
      width: parent.width
      visible: mobile.stations.length === 0 && !mobile.loadingStations
      title: mobile.online ? '没有找到电站' : '暂时无法加载电站'
      description: mobile.online ? '试试更换位置，或清除关键词与筛选。' : '请检查服务连接后重试。'
    }
  }
}
