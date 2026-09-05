import QtQuick 2.15
import QtQuick.Controls 2.15

Flickable {
  id: screen
  objectName: 'ordersPage'
  property string filter: 'all'
  property var filtered: {
    var rows = []
    for (var i = 0; i < mobile.orders.length; i++) {
      var order = mobile.orders[i]
      if (filter === 'all' || (filter === 'active' && ['reserved', 'charging', 'pending_payment'].indexOf(order.status) >= 0) || (filter === 'paid' && order.status === 'paid'))
        rows.push(order)
    }
    return rows
  }
  contentWidth: width
  contentHeight: content.height + 26
  boundsBehavior: Flickable.StopAtBounds
  clip: true
  ScrollBar.vertical: ScrollBar {
    policy: ScrollBar.AsNeeded
  }

  Column {
    id: content
    x: 22
    y: 10
    width: parent.width - 44
    spacing: 16
    Row {
      width: parent.width
      spacing: 8
      Repeater {
        model: [{
            "key": 'all',
            "label": '全部订单'
          }, {
            "key": 'active',
            "label": '进行中'
          }, {
            "key": 'paid',
            "label": '已完成'
          }]
        delegate: ActionButton {
          required property var modelData
          width: (content.width - 16) / 3
          height: 39
          text: modelData.label
          font.pixelSize: 12
          tone: screen.filter === modelData.key ? 'primary' : 'secondary'
          onClicked: screen.filter = modelData.key
        }
      }
    }
    Repeater {
      model: screen.filtered
      delegate: Rectangle {
        required property var modelData
        width: content.width
        height: cardContent.height + 38
        radius: 20
        color: 'white'
        border.color: Theme.border
        objectName: 'orderCard_' + modelData.id
        MouseArea {
          anchors.fill: parent
          onClicked: mobile.openOrder(Number(modelData.id))
        }
        Column {
          id: cardContent
          x: 18
          y: 19
          width: parent.width - 36
          spacing: 14
          Item {
            width: parent.width
            height: 26
            AppText {
              width: parent.width - statusBadge.width - 12
              text: modelData.stationName
              font.pixelSize: 16
              font.weight: Font.Bold
              elide: Text.ElideRight
              y: 2
            }
            Badge {
              id: statusBadge
              anchors.right: parent.right
              text: mobile.statusLabel(modelData.status)
              textColor: modelData.status === 'pending_payment' ? Theme.amber : modelData.status === 'cancelled' ? Theme.muted : Theme.primary
              fill: modelData.status === 'pending_payment' ? '#fff3dc' : modelData.status === 'cancelled' ? '#eff1ec' : Theme.primaryLight
            }
          }
          AppText {
            text: mobile.formatTime(modelData.createdAt) + '  ·  ' + modelData.chargerCode
            color: Theme.muted
            font.pixelSize: 11
          }
          Rectangle {
            width: parent.width
            height: 1
            color: Theme.border
          }
          Item {
            width: parent.width
            height: 36
            Column {
              spacing: 6
              AppText {
                text: Number(modelData.energyKwh || 0).toFixed(2) + ' 度 · ' + Math.floor(Number(modelData.durationSeconds || 0) / 60) + ' 分钟'
                color: Theme.muted
                font.pixelSize: 12
              }
              AppText {
                text: modelData.status === 'paid' || modelData.status === 'cancelled' ? '查看小票 →' : '继续处理 →'
                color: Theme.primary
                font.pixelSize: 11
              }
            }
            AppText {
              anchors.right: parent.right
              y: 5
              text: '¥' + (Number(modelData.amountCents || 0) / 100).toFixed(2)
              font.pixelSize: 24
              font.weight: Font.Bold
            }
          }
        }
      }
    }
    Item {
      width: 1
      height: 55
      visible: screen.filtered.length === 0
    }
    EmptyState {
      width: parent.width
      visible: screen.filtered.length === 0
      title: '还没有充电订单'
      description: '寻找附近的电站，开始你的第一次绿色补能。'
    }
    ActionButton {
      anchors.horizontalCenter: parent.horizontalCenter
      visible: screen.filtered.length === 0
      text: '去找电站'
      tone: 'secondary'
      onClicked: mobile.selectTab('home')
    }
  }
}
