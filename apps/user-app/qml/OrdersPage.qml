import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

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
    Row {
      width: parent.width
      spacing: Theme.space
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
          width: (content.width - Theme.space * 2) / 3
          text: modelData.label
          horizontalPadding: Theme.space
          tone: screen.filter === modelData.key ? 'primary' : 'secondary'
          onClicked: screen.filter = modelData.key
        }
      }
    }
    Repeater {
      model: screen.filtered
      delegate: Button {
        id: card
        required property var modelData
        objectName: 'orderCard_' + modelData.id
        width: content.width
        implicitHeight: details.implicitHeight + Theme.cardPadding * 2
        padding: Theme.cardPadding
        Accessible.name: modelData.stationName + '，' + mobile.statusLabel(modelData.status) + '，查看订单'
        onClicked: mobile.openOrder(Number(modelData.id))
        background: Rectangle {
          radius: Theme.cardRadius
          color: card.down ? Theme.primaryLight : Theme.card
          border.color: card.visualFocus ? Theme.primary : Theme.border
          border.width: card.visualFocus ? 2 : 1
        }
        contentItem: Column {
          id: details
          spacing: Theme.cardPadding
          RowLayout {
            width: parent.width
            spacing: Theme.space
            AppText {
              Layout.fillWidth: true
              text: card.modelData.stationName
              font.pixelSize: Theme.bodyLargeSize
              font.weight: Font.Medium
              elide: Text.ElideRight
            }
            Badge {
              text: mobile.statusLabel(card.modelData.status)
              textColor: card.modelData.status === 'pending_payment' ? Theme.amber : card.modelData.status === 'cancelled' ? Theme.muted : Theme.primary
              fill: card.modelData.status === 'pending_payment' ? '#fff3dc' : card.modelData.status === 'cancelled' ? '#eff1ec' : Theme.primaryLight
            }
          }
          Column {
            width: parent.width
            spacing: Theme.microSpace
            AppText {
              text: mobile.formatTime(card.modelData.createdAt)
              color: Theme.muted
              font.pixelSize: Theme.labelSize
            }
            AppText {
              text: card.modelData.chargerCode + ' · ' + Number(card.modelData.energyKwh || 0).toFixed(2) + ' 度 · ' + Math.floor(Number(card.modelData.durationSeconds || 0) / 60) + ' 分钟'
              color: Theme.muted
              font.pixelSize: Theme.labelSize
            }
          }
          Rectangle {
            width: parent.width
            height: 1
            color: Theme.border
          }
          RowLayout {
            width: parent.width
            MoneyText {
              Layout.fillWidth: true
              cents: Number(card.modelData.amountCents || 0)
            }
            AppText {
              text: card.modelData.status === 'paid' || card.modelData.status === 'cancelled' ? '查看小票' : '继续处理'
              color: Theme.primary
            }
            AppIcon {
              name: 'chevron-right'
              Layout.preferredWidth: 24
              Layout.preferredHeight: 24
            }
          }
        }
      }
    }
    Item {
      width: 1
      height: Theme.sectionSpace
      visible: screen.filtered.length === 0
    }
    EmptyState {
      width: parent.width
      visible: screen.filtered.length === 0
      title: '还没有充电订单'
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
