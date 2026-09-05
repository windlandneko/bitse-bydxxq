import QtQuick 2.15
import QtQuick.Controls 2.15

Flickable {
  objectName: 'stationPage'
  contentWidth: width
  contentHeight: content.height + 30
  boundsBehavior: Flickable.StopAtBounds
  clip: true
  ScrollBar.vertical: ScrollBar {
    policy: ScrollBar.AsNeeded
  }
  Column {
    id: content
    x: 22
    y: 8
    width: parent.width - 44
    spacing: 20
    Rectangle {
      width: parent.width
      height: header.height + 42
      radius: 25
      color: Theme.primary
      Column {
        id: header
        x: 22
        y: 22
        width: parent.width - 44
        spacing: 15
        Badge {
          text: mobile.station.region || '城市补能'
          fill: '#42785c'
          textColor: '#e2efdc'
        }
        AppText {
          width: parent.width
          text: mobile.station.name || ''
          font.pixelSize: 25
          font.weight: Font.Bold
          color: 'white'
          wrapMode: Text.WordWrap
        }
        AppText {
          width: parent.width
          text: mobile.station.address || ''
          color: '#bfd6c6'
          font.pixelSize: 12
          wrapMode: Text.WordWrap
          lineHeight: 1.4
        }
        Row {
          width: parent.width
          spacing: 22
          Column {
            spacing: 6
            AppText {
              text: '¥' + (Number(mobile.station.priceCents || 0) / 100).toFixed(2)
              color: Theme.accent
              font.pixelSize: 30
              font.weight: Font.Bold
            }
            AppText {
              text: '每度电 · 综合电价'
              color: '#b7d0c0'
              font.pixelSize: 10
            }
          }
          Rectangle {
            width: 1
            height: 46
            color: '#54826a'
            y: 5
          }
          Column {
            spacing: 6
            AppText {
              text: (mobile.station.idleChargers || 0) + ' / ' + (mobile.station.totalChargers || 0)
              color: 'white'
              font.pixelSize: 30
              font.weight: Font.Bold
            }
            AppText {
              text: '空闲电桩 / 全部'
              color: '#b7d0c0'
              font.pixelSize: 10
            }
          }
        }
        ActionButton {
          objectName: 'stationNavigationButton'
          width: parent.width
          height: 43
          tone: 'secondary'
          text: '距离 ' + Number(mobile.station.distanceKm || 0).toFixed(1) + ' km  ·  开始导航 ↗'
          font.pixelSize: 13
          onClicked: mobile.openNavigation(mobile.station)
        }
      }
    }
    Rectangle {
      width: parent.width
      height: forecastNote.implicitHeight + 26
      visible: !!mobile.station.forecastAt
      radius: 15
      color: '#edf1df'
      AppText {
        id: forecastNote
        x: 15
        y: 13
        width: parent.width - 30
        text: '空闲预测  ·  1 小时后预计有 ' + Number(mobile.station.predictedAvailableChargers || 0) + ' 个空闲电桩。实际可用数量以当前状态为准。'
        font.pixelSize: 12
        color: '#687b46'
        wrapMode: Text.WordWrap
        lineHeight: 1.5
      }
    }
    Item {
      width: parent.width
      height: 30
      AppText {
        text: '选择充电桩'
        font.pixelSize: 19
        font.weight: Font.Bold
      }
      AppText {
        text: '快充 DC / 慢充 AC'
        anchors.right: parent.right
        y: 6
        font.pixelSize: 11
        color: Theme.muted
      }
    }
    Repeater {
      model: mobile.chargers
      delegate: Rectangle {
        required property var modelData
        width: content.width
        height: 116
        radius: 20
        color: 'white'
        border.color: modelData.status === 'idle' ? '#d9e5d8' : Theme.border
        objectName: 'chargerCard_' + modelData.id
        Row {
          x: 17
          y: 17
          width: parent.width - 34
          spacing: 12
          Rectangle {
            width: 42
            height: 42
            radius: 13
            color: modelData.type === 'dc' ? Theme.primaryLight : '#f1eee5'
            AppText {
              anchors.centerIn: parent
              text: modelData.type === 'dc' ? '快' : '慢'
              font.pixelSize: 17
              font.weight: Font.Bold
              color: modelData.type === 'dc' ? Theme.primary : '#8c784b'
            }
          }
          Column {
            width: parent.width - 54
            spacing: 5
            AppText {
              text: modelData.code
              font.pixelSize: 15
              font.weight: Font.Bold
            }
            AppText {
              text: (modelData.type === 'dc' ? '直流快充' : '交流慢充') + '  ·  ' + modelData.powerKw + ' kW'
              color: Theme.muted
              font.pixelSize: 12
            }
          }
        }
        Badge {
          x: 18
          y: 79
          text: mobile.statusLabel(modelData.status)
          textColor: modelData.status === 'idle' ? Theme.primary : modelData.status === 'fault' ? Theme.danger : Theme.muted
          fill: modelData.status === 'idle' ? Theme.primaryLight : modelData.status === 'fault' ? Theme.dangerLight : '#f1f2ef'
        }
        ActionButton {
          anchors.right: parent.right
          anchors.rightMargin: 16
          y: 70
          width: 113
          height: 34
          text: modelData.status === 'idle' ? '预约充电 →' : '暂不可用'
          font.pixelSize: 12
          enabled: modelData.status === 'idle' && !mobile.busy
          objectName: 'reserveCharger_' + modelData.id
          onClicked: mobile.reserve(Number(modelData.id))
        }
      }
    }
    EmptyState {
      width: parent.width
      visible: mobile.chargers.length === 0
      title: '电桩信息正在准备'
      description: '稍后点击右上角刷新，获取本站电桩状态。'
    }
    AppText {
      width: parent.width
      text: '预约成功后保留 15 分钟，开始充电后按实际电量计费。'
      color: Theme.muted
      font.pixelSize: 11
      horizontalAlignment: Text.AlignHCenter
      wrapMode: Text.WordWrap
      lineHeight: 1.4
    }
  }
}
