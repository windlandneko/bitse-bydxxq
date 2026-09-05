import QtQuick 2.15
import QtQuick.Controls 2.15

Flickable {
  id: screen
  objectName: 'receiptPage'
  property var order: mobile.viewedOrder
  contentWidth: width
  contentHeight: content.height + 32
  boundsBehavior: Flickable.StopAtBounds
  clip: true
  ScrollBar.vertical: ScrollBar {
    policy: ScrollBar.AsNeeded
  }
  Column {
    id: content
    x: 22
    y: 12
    width: parent.width - 44
    spacing: 23
    Rectangle {
      anchors.horizontalCenter: parent.horizontalCenter
      width: 66
      height: 66
      radius: 25
      color: screen.order.status === 'paid' ? Theme.primaryLight : '#eceee8'
      AppText {
        anchors.centerIn: parent
        text: screen.order.status === 'paid' ? '✓' : '−'
        font.pixelSize: 33
        color: Theme.primary
      }
    }
    Column {
      width: parent.width
      spacing: 8
      AppText {
        width: parent.width
        text: screen.order.status === 'paid' ? '充电完成，一路顺风' : '预约已取消'
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: 23
        font.weight: Font.Bold
      }
      AppText {
        width: parent.width
        text: screen.order.status === 'paid' ? '感谢选择智充出行' : '本次预约未产生费用'
        horizontalAlignment: Text.AlignHCenter
        color: Theme.muted
        font.pixelSize: 12
      }
    }
    Rectangle {
      width: parent.width
      height: receipt.height + 48
      radius: 24
      color: 'white'
      Column {
        id: receipt
        x: 23
        y: 24
        width: parent.width - 46
        spacing: 18
        AppText {
          anchors.horizontalCenter: parent.horizontalCenter
          text: '实付金额'
          color: Theme.muted
          font.pixelSize: 12
        }
        AppText {
          anchors.horizontalCenter: parent.horizontalCenter
          text: '¥' + (Number(screen.order.amountCents || 0) / 100).toFixed(2)
          font.pixelSize: 43
          font.weight: Font.Bold
          color: Theme.primary
        }
        Rectangle {
          width: parent.width
          height: 1
          color: Theme.border
        }
        KeyValue {
          width: parent.width
          label: '电站名称'
          value: screen.order.stationName || ''
        }
        KeyValue {
          width: parent.width
          label: '充电桩'
          value: screen.order.chargerCode || ''
        }
        KeyValue {
          width: parent.width
          label: '充电电量'
          value: Number(screen.order.energyKwh || 0).toFixed(3) + ' 度'
        }
        KeyValue {
          width: parent.width
          label: '充电时长'
          value: Math.floor(Number(screen.order.durationSeconds || 0) / 60) + ' 分 ' + Number(screen.order.durationSeconds || 0) % 60 + ' 秒'
        }
        KeyValue {
          width: parent.width
          label: '电价'
          value: '¥' + (Number(screen.order.priceCents || 0) / 100).toFixed(2) + ' / 度'
        }
        KeyValue {
          width: parent.width
          label: '开始时间'
          value: mobile.formatTime(screen.order.startedAt || '')
        }
        KeyValue {
          width: parent.width
          label: '结束时间'
          value: mobile.formatTime(screen.order.endedAt || '')
        }
        KeyValue {
          width: parent.width
          label: '订单状态'
          value: mobile.statusLabel(screen.order.status || '')
          valueColor: Theme.primary
        }
        Rectangle {
          width: parent.width
          height: 1
          color: Theme.border
        }
        Column {
          width: parent.width
          spacing: 8
          AppText {
            text: '订单编号'
            color: Theme.muted
            font.pixelSize: 10
          }
          AppText {
            width: parent.width
            text: screen.order.orderNo || ''
            color: Theme.muted
            font.pixelSize: 11
            wrapMode: Text.WrapAnywhere
          }
        }
      }
      Rectangle {
        x: -7
        y: 131
        width: 14
        height: 14
        radius: 7
        color: Theme.paper
      }
      Rectangle {
        anchors.right: parent.right
        anchors.rightMargin: -7
        y: 131
        width: 14
        height: 14
        radius: 7
        color: Theme.paper
      }
    }
    ActionButton {
      objectName: 'receiptDoneButton'
      width: parent.width
      text: '完成'
      onClicked: mobile.selectTab('orders')
    }
  }
}
