import QtQuick 2.15
import QtQuick.Controls 2.15

Loader {
  objectName: 'receiptPage'
  active: mobile.viewedOrder.id !== undefined
  sourceComponent: Item {
    id: screen
    readonly property var order: mobile.viewedOrder
    Flickable {
      anchors.fill: parent
      anchors.bottomMargin: 80
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
          anchors.horizontalCenter: parent.horizontalCenter
          width: 48
          height: 48
          radius: Theme.heroRadius
          color: Theme.primaryLight
          AppIcon {
            anchors.centerIn: parent
            name: screen.order.status === 'paid' ? 'circle-check' : 'x'
          }
        }
        Column {
          width: parent.width
          spacing: Theme.space
          AppText {
            width: parent.width
            text: screen.order.status === 'paid' ? '充电完成' : '预约已取消'
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.titleSize
            font.weight: Font.DemiBold
          }
          AppText {
            width: parent.width
            visible: screen.order.status !== 'paid'
            text: '本次预约未产生费用'
            horizontalAlignment: Text.AlignHCenter
            color: Theme.muted
          }
        }
        Rectangle {
          width: parent.width
          height: receipt.height + Theme.cardPadding * 2
          radius: Theme.cardRadius
          color: Theme.card
          Column {
            id: receipt
            x: Theme.cardPadding
            y: Theme.cardPadding
            width: parent.width - Theme.cardPadding * 2
            spacing: Theme.cardPadding
            Column {
              width: parent.width
              spacing: Theme.space
              AppText {
                anchors.horizontalCenter: parent.horizontalCenter
                text: '实付金额'
                color: Theme.muted
                font.pixelSize: Theme.labelSize
              }
              MoneyText {
                anchors.horizontalCenter: parent.horizontalCenter
                cents: screen.order.amountCents
                valueSize: 40
              }
            }
            Rectangle {
              width: parent.width
              height: 1
              color: Theme.border
            }
            Column {
              width: parent.width
              spacing: Theme.space
              KeyValue {
                width: parent.width
                label: '电站名称'
                value: screen.order.stationName
              }
              KeyValue {
                width: parent.width
                label: '充电桩'
                value: screen.order.chargerCode
              }
              KeyValue {
                width: parent.width
                label: '充电电量'
                value: screen.order.energyKwh.toFixed(3) + ' 度'
              }
              KeyValue {
                width: parent.width
                label: '充电时长'
                value: Math.floor(screen.order.durationSeconds / 60) + ' 分 ' + screen.order.durationSeconds % 60 + ' 秒'
              }
              KeyValue {
                width: parent.width
                label: '电价'
                value: '¥' + (screen.order.priceCents / 100).toFixed(2) + ' / 度'
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
                value: mobile.statusLabel(screen.order.status)
                valueColor: Theme.primary
              }
            }
            Rectangle {
              width: parent.width
              height: 1
              color: Theme.border
            }
            Column {
              width: parent.width
              spacing: Theme.space
              AppText {
                text: '订单编号'
                color: Theme.muted
                font.pixelSize: Theme.labelSize
              }
              AppText {
                width: parent.width
                text: screen.order.orderNo
                color: Theme.muted
                font.pixelSize: Theme.labelSize
                wrapMode: Text.WrapAnywhere
              }
            }
          }
        }
      }
    }
    Rectangle {
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottom: parent.bottom
      height: 80
      color: Theme.paper
      Rectangle {
        width: parent.width
        height: 1
        color: Theme.border
      }
      ActionButton {
        objectName: 'receiptDoneButton'
        anchors.fill: parent
        anchors.margins: Theme.pagePadding
        text: '完成'
        onClicked: mobile.selectTab('orders')
      }
    }
  }
}
