import QtQuick 2.15
import QtQuick.Controls 2.15

Flickable {
  objectName: 'rechargePage'
  contentWidth: width
  contentHeight: content.height + 30
  boundsBehavior: Flickable.StopAtBounds
  clip: true
  Column {
    id: content
    x: 22
    y: 10
    width: parent.width - 44
    spacing: 25
    Rectangle {
      width: parent.width
      height: 132
      radius: 24
      color: Theme.primary
      Column {
        x: 24
        y: 24
        spacing: 14
        AppText {
          text: '当前钱包余额'
          color: '#c2d8c8'
          font.pixelSize: 12
        }
        AppText {
          text: '¥ ' + (Number(mobile.user.balanceCents || 0) / 100).toFixed(2)
          color: 'white'
          font.pixelSize: 36
          font.weight: Font.Bold
        }
      }
    }
    Column {
      width: parent.width
      spacing: 15
      AppText {
        text: '选择充值金额'
        font.pixelSize: 19
        font.weight: Font.Bold
      }
      Flow {
        width: parent.width
        spacing: 10
        Repeater {
          model: ['20', '50', '100', '200', '500', '1000']
          delegate: ActionButton {
            required property string modelData
            width: (content.width - 20) / 3
            height: 64
            text: '¥ ' + modelData
            tone: amount.text === modelData ? 'primary' : 'secondary'
            onClicked: amount.text = modelData
          }
        }
      }
    }
    Column {
      width: parent.width
      spacing: 11
      AppText {
        text: '自定义金额（元）'
        font.pixelSize: 14
        font.weight: Font.DemiBold
      }
      AppField {
        id: amount
        objectName: 'rechargeAmountInput'
        width: parent.width
        text: '50'
        placeholderText: '0.01 — 10,000.00'
        inputMethodHints: Qt.ImhFormattedNumbersOnly
        maximumLength: 8
        validator: RegularExpressionValidator {
          regularExpression: /[0-9]{0,5}(\.[0-9]{0,2})?/
        }
        onAccepted: mobile.recharge(text)
      }
      AppText {
        text: '支持精确到分，到账后余额实时更新'
        color: Theme.muted
        font.pixelSize: 11
      }
    }
    ActionButton {
      objectName: 'confirmRechargeButton'
      width: parent.width
      enabled: !mobile.busy
      text: mobile.busy ? '正在充值…' : '确认充值  ¥' + (Number(amount.text || 0)).toFixed(2)
      onClicked: mobile.recharge(amount.text)
    }
    Rectangle {
      width: parent.width
      height: note.implicitHeight + 32
      radius: 17
      color: '#eff1e8'
      AppText {
        id: note
        x: 16
        y: 16
        width: parent.width - 32
        text: '这是课程演示的模拟充值，不会产生真实扣款。充值余额可用于预约和支付充电订单。'
        color: '#87917b'
        font.pixelSize: 12
        wrapMode: Text.WordWrap
        lineHeight: 1.5
      }
    }
  }
}
