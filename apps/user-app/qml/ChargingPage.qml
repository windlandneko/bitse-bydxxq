import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
  id: screen
  objectName: mobile.page === 'settlement' ? 'settlementPage' : 'chargingPage'
  property var order: mobile.activeOrder
  property string stateName: order.status || ''
  property real stateOfCharge: Number(order.soc || 0)
  Flickable {
    id: scroll
    anchors.fill: parent
    anchors.bottomMargin: actionDock.height
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
      Column {
        width: parent.width
        spacing: 8
        AppText {
          anchors.horizontalCenter: parent.horizontalCenter
          text: screen.stateName === 'reserved' ? '电桩已为你预留' : screen.stateName === 'charging' ? '正在补充美好能量' : '本次充电已结束'
          font.pixelSize: 24
          font.weight: Font.Bold
        }
        AppText {
          width: parent.width
          text: screen.order.stationName || ''
          horizontalAlignment: Text.AlignHCenter
          font.pixelSize: 12
          color: Theme.muted
          elide: Text.ElideRight
        }
      }

      Item {
        width: parent.width
        height: 210
        Rectangle {
          anchors.centerIn: parent
          width: 197
          height: 197
          radius: 100
          color: '#edf2e7'
          Rectangle {
            anchors.centerIn: parent
            width: 156
            height: 156
            radius: 80
            color: '#e3ecd9'
          }
        }
        Canvas {
          id: progress
          anchors.centerIn: parent
          width: 215
          height: 215
          property real value: screen.stateName === 'reserved' ? 1 : Math.min(1, screen.stateOfCharge / 100)
          onValueChanged: requestPaint()
          onPaint: {
            var ctx = getContext('2d')
            ctx.clearRect(0, 0, width, height)
            ctx.lineWidth = 8
            ctx.strokeStyle = '#dce6d6'
            ctx.beginPath()
            ctx.arc(width / 2, height / 2, 100, 0, Math.PI * 2)
            ctx.stroke()
            ctx.strokeStyle = '#508e56'
            ctx.lineCap = 'round'
            ctx.beginPath()
            ctx.arc(width / 2, height / 2, 100, -Math.PI / 2, Math.PI * 2 * value - Math.PI / 2)
            ctx.stroke()
          }
        }
        Column {
          anchors.centerIn: parent
          spacing: 6
          AppText {
            anchors.horizontalCenter: parent.horizontalCenter
            text: screen.stateName === 'reserved' ? '预约保留中' : '电池电量'
            color: '#6f876a'
            font.pixelSize: 12
          }
          AppText {
            anchors.horizontalCenter: parent.horizontalCenter
            text: screen.stateName === 'reserved' ? (mobile.clockMs > 0 ? mobile.reservationRemaining() : '') : Math.round(screen.stateOfCharge) + '%'
            color: Theme.primary
            font.pixelSize: 43
            font.weight: Font.Bold
          }
          Badge {
            anchors.horizontalCenter: parent.horizontalCenter
            text: mobile.statusLabel(screen.stateName)
            fill: '#d4e4c7'
            textColor: '#4c7240'
          }
        }
      }

      Rectangle {
        width: parent.width
        height: 103
        radius: 21
        color: 'white'
        Row {
          anchors.fill: parent
          anchors.margins: 18
          Repeater {
            model: [{
                "label": '已充电量',
                "value": Number(screen.order.energyKwh || 0).toFixed(2),
                "unit": '度'
              }, {
                "label": '充电时长',
                "value": Math.floor(Number(screen.order.durationSeconds || 0) / 60).toString(),
                "unit": '分钟'
              }, {
                "label": '当前费用',
                "value": (Number(screen.order.amountCents || 0) / 100).toFixed(2),
                "unit": '元'
              }]
            delegate: Column {
              required property var modelData
              width: (content.width - 36) / 3
              spacing: 7
              AppText {
                anchors.horizontalCenter: parent.horizontalCenter
                text: modelData.label
                font.pixelSize: 11
                color: Theme.muted
              }
              AppText {
                anchors.horizontalCenter: parent.horizontalCenter
                text: modelData.value
                font.pixelSize: 23
                font.weight: Font.Bold
              }
              AppText {
                anchors.horizontalCenter: parent.horizontalCenter
                text: modelData.unit
                font.pixelSize: 10
                color: Theme.muted
              }
            }
          }
        }
      }

      Rectangle {
        width: parent.width
        height: detail.height + 38
        radius: 20
        color: 'white'
        Column {
          id: detail
          x: 18
          y: 19
          width: parent.width - 36
          spacing: 15
          KeyValue {
            width: parent.width
            label: '充电桩'
            value: screen.order.chargerCode || ''
          }
          KeyValue {
            width: parent.width
            label: '充电类型'
            value: (screen.order.chargerType === 'dc' ? '直流快充' : '交流慢充') + ' · ' + (screen.order.powerKw || 0) + ' kW'
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
            label: '钱包余额'
            value: '¥' + (Number(mobile.user.balanceCents || 0) / 100).toFixed(2)
            valueColor: Theme.primary
          }
        }
      }

      Rectangle {
        width: parent.width
        height: chargingNote.implicitHeight + 28
        radius: 15
        color: '#edeedc'
        AppText {
          id: chargingNote
          x: 15
          y: 14
          width: parent.width - 30
          text: screen.stateName === 'reserved' ? '请在预约保留时间内连接车辆并开始充电。超时将自动取消预约。' : screen.stateName === 'charging' ? '充电持续进行，退出应用不会停止计费。电池充满或可用余额耗尽时会自动结束。' : (screen.order.stopReason ? screen.order.stopReason + '。' : '') + '请确认本次用电明细，费用将从钱包余额扣除。'
          font.pixelSize: 12
          color: '#7b7d4e'
          wrapMode: Text.WordWrap
          lineHeight: 1.5
        }
      }
    }
  }

  Rectangle {
    id: actionDock
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.bottom: parent.bottom
    height: actionColumn.height + 24
    color: Theme.paper
    Rectangle {
      width: parent.width
      height: 1
      color: Theme.border
    }
    Column {
      id: actionColumn
      x: 22
      y: 12
      width: parent.width - 44
      spacing: 5
      ActionButton {
        objectName: 'startChargingButton'
        width: parent.width
        visible: screen.stateName === 'reserved'
        enabled: !mobile.busy
        text: '已连接车辆，开始充电'
        onClicked: mobile.startCharging()
      }
      ActionButton {
        objectName: 'stopChargingButton'
        width: parent.width
        visible: screen.stateName === 'charging'
        enabled: !mobile.busy
        text: mobile.page === 'settlement' ? '结束充电，确认费用' : '结束充电，前往结算'
        onClicked: {
          confirmation.action = 'stop'
          confirmation.open()
        }
      }
      ActionButton {
        objectName: 'settleOrderButton'
        width: parent.width
        visible: screen.stateName === 'pending_payment'
        enabled: !mobile.busy
        text: '确认支付  ¥' + (Number(screen.order.amountCents || 0) / 100).toFixed(2)
        onClicked: mobile.settle()
      }
      ActionButton {
        objectName: 'cancelReservationButton'
        width: parent.width
        visible: screen.stateName === 'reserved'
        enabled: !mobile.busy
        text: '取消预约'
        tone: 'quiet'
        onClicked: {
          confirmation.action = 'cancel'
          confirmation.open()
        }
      }
    }
  }

  Popup {
    id: confirmation
    property string action: 'stop'
    anchors.centerIn: Overlay.overlay
    width: screen.width - 48
    padding: 24
    modal: true
    background: Rectangle {
      color: 'white'
      radius: 25
    }
    Overlay.modal: Rectangle {
      color: '#75102017'
    }
    contentItem: Column {
      spacing: 19
      AppText {
        text: confirmation.action === 'stop' ? '结束本次充电？' : '取消本次预约？'
        font.pixelSize: 22
        font.weight: Font.Bold
      }
      AppText {
        width: parent.width
        text: confirmation.action === 'stop' ? '充电结束后释放电桩，并按实际充电量生成结算金额。' : '取消后将释放电桩，本次预约不会产生费用。'
        color: Theme.muted
        wrapMode: Text.WordWrap
        lineHeight: 1.5
      }
      ActionButton {
        objectName: 'confirmOrderActionButton'
        width: parent.width
        text: confirmation.action === 'stop' ? '确认结束' : '确认取消'
        onClicked: {
          confirmation.close()
          if (confirmation.action === 'stop')
            mobile.stopCharging()
          else
            mobile.cancelReservation()
        }
      }
      ActionButton {
        width: parent.width
        text: '继续保留'
        tone: 'quiet'
        onClicked: confirmation.close()
      }
    }
  }
}
