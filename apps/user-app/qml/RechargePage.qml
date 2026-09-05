import QtQuick 2.15
import QtQuick.Controls 2.15

Flickable {
  objectName: 'rechargePage'
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
    spacing: Theme.sectionSpace
    Rectangle {
      width: parent.width
      height: balance.height + Theme.cardPadding * 2
      radius: Theme.heroRadius
      color: Theme.primary
      Column {
        id: balance
        x: Theme.cardPadding
        y: Theme.cardPadding
        width: parent.width - Theme.cardPadding * 2
        spacing: Theme.space
        AppText {
          text: '当前钱包余额'
          color: '#d2e2d5'
        }
        MoneyText {
          cents: Number(mobile.user.balanceCents || 0)
          valueColor: 'white'
          valueSize: 36
        }
      }
    }
    Column {
      width: parent.width
      spacing: Theme.cardPadding
      AppText {
        text: '选择充值金额'
        font.pixelSize: Theme.bodyLargeSize
        font.weight: Font.Medium
      }
      Flow {
        width: parent.width
        spacing: Theme.space
        Repeater {
          model: ['20', '50', '100', '200', '500', '1000']
          delegate: ActionButton {
            required property string modelData
            width: (content.width - Theme.space * 2) / 3
            height: 64
            text: '¥ ' + modelData
            font.pixelSize: Theme.bodyLargeSize
            tone: amount.text === modelData ? 'primary' : 'secondary'
            onClicked: amount.text = modelData
          }
        }
      }
    }
    Column {
      width: parent.width
      spacing: Theme.space
      AppText {
        text: '自定义金额（元）'
        font.pixelSize: Theme.bodyLargeSize
        font.weight: Font.Medium
      }
      AppField {
        id: amount
        objectName: 'rechargeAmountInput'
        width: parent.width
        text: '50'
        placeholderText: '0.01 — 10,000.00'
        Accessible.name: '自定义充值金额，单位元'
        inputMethodHints: Qt.ImhFormattedNumbersOnly
        maximumLength: 8
        validator: RegularExpressionValidator {
          regularExpression: /[0-9]{0,5}(\.[0-9]{0,2})?/
        }
        onAccepted: mobile.recharge(text)
      }
      AppText {
        text: '到账后余额实时更新'
        color: Theme.muted
        font.pixelSize: Theme.labelSize
      }
    }
    ActionButton {
      objectName: 'confirmRechargeButton'
      width: parent.width
      enabled: !mobile.busy
      text: mobile.busy ? '正在充值…' : '确认充值 ¥' + Number(amount.text || 0).toFixed(2)
      onClicked: mobile.recharge(amount.text)
    }
    Rectangle {
      width: parent.width
      height: note.implicitHeight + Theme.cardPadding * 2
      radius: Theme.cardRadius
      color: '#eff1e8'
      AppText {
        id: note
        x: Theme.cardPadding
        y: Theme.cardPadding
        width: parent.width - Theme.cardPadding * 2
        text: '课程演示的模拟充值，不会产生真实扣款。余额可用于预约和支付充电订单。'
        color: Theme.muted
        wrapMode: Text.WordWrap
        lineHeight: 1.5
      }
    }
  }
}
