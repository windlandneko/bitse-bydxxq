import QtQuick 2.15
import QtQuick.Controls 2.15

Flickable {
  objectName: 'loginPage'
  contentWidth: width
  contentHeight: content.height + 48
  clip: true
  boundsBehavior: Flickable.StopAtBounds

  Column {
    id: content
    x: 28
    y: 34
    width: parent.width - 56
    spacing: 26
    Row {
      spacing: 12
      Image {
        source: 'qrc:/assets/brand.svg'
        width: 46
        height: 46
      }
      Column {
        y: 2
        spacing: 4
        AppText {
          text: '智充出行'
          font.pixelSize: 21
          font.weight: Font.Bold
        }
        AppText {
          text: 'CHARGE & GO'
          color: Theme.muted
          font.pixelSize: 10
          font.letterSpacing: 2
        }
      }
    }

    Rectangle {
      width: parent.width
      height: 285
      radius: 30
      color: Theme.primary
      clip: true
      Rectangle {
        x: parent.width - 230
        y: 44
        width: 210
        height: 210
        radius: 105
        color: '#2b674d'
      }
      Rectangle {
        x: parent.width - 162
        y: 108
        width: 140
        height: 140
        radius: 70
        color: '#327455'
      }
      Column {
        x: 26
        y: 29
        spacing: 12
        AppText {
          text: '每一程，\n都电力满满。'
          color: 'white'
          font.pixelSize: 31
          font.weight: Font.Bold
          lineHeight: 1.2
        }
        AppText {
          text: '好电站，就在你身边'
          color: '#c0d7c8'
          font.pixelSize: 13
        }
      }
      // A small vector charging scene stays crisp at every simulated phone size.
      Item {
        width: 176
        height: 110
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 22
        anchors.bottomMargin: 22
        Rectangle {
          x: 4
          y: 88
          width: 171
          height: 2
          color: '#77a389'
        }
        Rectangle {
          x: 113
          y: 3
          width: 41
          height: 84
          radius: 9
          color: Theme.accent
        }
        Rectangle {
          x: 121
          y: 14
          width: 25
          height: 21
          radius: 4
          color: '#27593f'
        }
        AppText {
          x: 128
          y: 37
          text: 'ϟ'
          font.pixelSize: 26
          color: Theme.primary
        }
        Rectangle {
          x: 4
          y: 44
          width: 94
          height: 34
          radius: 14
          color: '#f0f5e7'
        }
        Rectangle {
          x: 20
          y: 23
          width: 54
          height: 34
          radius: 16
          color: '#f0f5e7'
        }
        Rectangle {
          x: 27
          y: 29
          width: 40
          height: 21
          radius: 8
          color: '#78a991'
        }
        Rectangle {
          x: 8
          y: 67
          width: 24
          height: 24
          radius: 12
          color: '#153a29'
          border.width: 6
          border.color: '#a3baa5'
        }
        Rectangle {
          x: 66
          y: 67
          width: 24
          height: 24
          radius: 12
          color: '#153a29'
          border.width: 6
          border.color: '#a3baa5'
        }
        Rectangle {
          x: 151
          y: 22
          width: 9
          height: 5
          radius: 2
          color: '#d9ee89'
        }
        Rectangle {
          x: 157
          y: 23
          width: 5
          height: 50
          radius: 2
          color: '#d9ee89'
        }
        Rectangle {
          x: 150
          y: 69
          width: 12
          height: 5
          radius: 2
          color: '#d9ee89'
        }
      }
      Badge {
        x: 26
        y: 234
        text: '绿色出行'
        fill: '#46795b'
        textColor: '#e9f3df'
      }
    }

    Column {
      width: parent.width
      spacing: 10
      AppText {
        text: '欢迎使用智充出行'
        font.pixelSize: 23
        font.weight: Font.Bold
      }
      AppText {
        text: '手机号即可登录，首次使用自动创建账号'
        color: Theme.muted
        font.pixelSize: 12
      }
    }
    Column {
      width: parent.width
      spacing: 12
      AppField {
        id: phone
        objectName: 'phoneInput'
        width: parent.width
        placeholderText: '请输入 11 位手机号'
        maximumLength: 11
        inputMethodHints: Qt.ImhDigitsOnly
        validator: RegularExpressionValidator {
          regularExpression: /[0-9]{0,11}/
        }
        onAccepted: mobile.login(text)
        Accessible.name: '手机号'
      }
      ActionButton {
        objectName: 'loginButton'
        width: parent.width
        text: mobile.busy ? '正在登录…' : '登录 / 注册  →'
        enabled: !mobile.busy
        onClicked: mobile.login(phone.text)
      }
      AppText {
        width: parent.width
        text: '课程演示 · 手机号免密登录 · 模拟充值'
        color: '#929d94'
        font.pixelSize: 11
        horizontalAlignment: Text.AlignHCenter
      }
    }
  }
}
