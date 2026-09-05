import QtQuick 2.15
import QtQuick.Controls 2.15

Flickable {
  id: screen
  objectName: 'homePage'
  contentWidth: width
  contentHeight: content.height + 28
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
    x: 22
    y: 8
    width: parent.width - 44
    spacing: 18
    Rectangle {
      width: parent.width
      height: 46
      radius: 14
      color: Theme.primaryLight
      Row {
        x: 14
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8
        Image {
          width: 17
          height: 17
          y: 1
          source: 'qrc:/icons/navigation.svg'
        }
        AppText {
          width: content.width - 78
          text: mobile.locationName
          font.pixelSize: 13
          elide: Text.ElideRight
          color: Theme.primary
        }
        AppText {
          text: '⌄'
          color: Theme.primary
          font.pixelSize: 16
        }
      }
      MouseArea {
        objectName: 'chooseLocationButton'
        anchors.fill: parent
        onClicked: mobile.navigate('location')
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

    Rectangle {
      width: parent.width
      height: activeInfo.height + 32
      visible: Number(mobile.activeOrder.id || 0) > 0
      radius: 17
      color: '#f0eddc'
      border.color: '#e8dfba'
      Column {
        id: activeInfo
        x: 16
        y: 16
        width: parent.width - 42
        spacing: 6
        AppText {
          text: '你有一笔' + mobile.statusLabel(mobile.activeOrder.status || '') + '订单'
          font.weight: Font.Bold
          color: '#796230'
        }
        AppText {
          text: mobile.activeOrder.stationName || ''
          width: parent.width
          elide: Text.ElideRight
          color: '#9d8658'
          font.pixelSize: 12
        }
      }
      AppText {
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        text: '›'
        font.pixelSize: 25
        color: '#796230'
      }
      MouseArea {
        anchors.fill: parent
        objectName: 'activeOrderBanner'
        onClicked: mobile.openActiveOrder()
      }
    }

    Rectangle {
      width: parent.width
      height: 126
      radius: 23
      color: Theme.primary
      clip: true
      Rectangle {
        x: parent.width - 117
        y: 13
        width: 100
        height: 100
        radius: 50
        color: '#326d50'
      }
      Rectangle {
        x: parent.width - 91
        y: 37
        width: 67
        height: 67
        radius: 34
        color: '#418262'
      }
      Column {
        x: 22
        y: 23
        spacing: 9
        AppText {
          text: '好电站，近一点'
          color: 'white'
          font.pixelSize: 23
          font.weight: Font.Bold
        }
        AppText {
          text: '实时空闲 · 透明电价 · 便捷预约'
          color: '#c5d9c9'
          font.pixelSize: 11
        }
        Row {
          spacing: 5
          Rectangle {
            width: 5
            height: 5
            radius: 3
            color: Theme.accent
            y: 5
          }
          AppText {
            text: '已找到 ' + mobile.stations.length + ' 座电站'
            color: Theme.accent
            font.pixelSize: 11
          }
        }
      }
      AppText {
        anchors.right: parent.right
        anchors.rightMargin: 27
        y: 34
        text: 'ϟ'
        font.pixelSize: 63
        font.weight: Font.Bold
        color: Theme.accent
        rotation: 12
      }
    }

    Column {
      width: parent.width
      spacing: 10
      visible: screen.suggestion !== null
      Item {
        width: parent.width
        height: 25
        AppText {
          text: '为你推荐'
          font.pixelSize: 17
          font.weight: Font.Bold
        }
        AppText {
          anchors.right: parent.right
          anchors.verticalCenter: parent.verticalCenter
          text: '根据未来空闲预测'
          font.pixelSize: 10
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
      spacing: 12
      Item {
        width: parent.width
        height: 26
        AppText {
          text: '附近电站'
          font.pixelSize: 18
          font.weight: Font.Bold
        }
        AppText {
          anchors.right: parent.right
          y: 4
          text: mobile.loadingStations ? '正在更新…' : mobile.stations.length + ' 座可选'
          font.pixelSize: 11
          color: Theme.muted
        }
      }
      Row {
        width: parent.width
        spacing: 6
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
            width: (content.width - 78 - 18) / 3
            height: 35
            font.pixelSize: 11
            horizontalPadding: 4
            text: modelData.text
            tone: mobile.sort === modelData.key ? 'primary' : 'secondary'
            onClicked: mobile.sort = modelData.key
          }
        }
        ActionButton {
          objectName: 'fastOnlyButton'
          width: 78
          height: 35
          text: '仅快充'
          font.pixelSize: 11
          horizontalPadding: 4
          tone: mobile.fastOnly ? 'primary' : 'secondary'
          onClicked: mobile.fastOnly = !mobile.fastOnly
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
      title: mobile.online ? '暂时没有找到电站' : '连接后即可发现电站'
      description: mobile.online ? '试试更换位置，或清除关键词和快充筛选。' : '请确认充电服务已启动，然后点击右上角刷新。'
    }
  }
}
