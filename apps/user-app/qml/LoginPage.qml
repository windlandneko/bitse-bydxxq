import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Flickable {
  objectName: 'loginPage'
  contentWidth: width
  contentHeight: content.height + Theme.sectionSpace * 2
  clip: true
  boundsBehavior: Flickable.StopAtBounds
  Column {
    id: content
    x: Theme.pagePadding
    y: Theme.sectionSpace
    width: parent.width - Theme.pagePadding * 2
    spacing: Theme.sectionSpace
    RowLayout {
      width: parent.width
      spacing: Theme.cardPadding
      Image {
        source: 'qrc:/assets/brand.svg'
        Layout.preferredWidth: 48
        Layout.preferredHeight: 48
        Accessible.ignored: true
      }
      AppText {
        Layout.fillWidth: true
        text: '智充出行'
        font.pixelSize: Theme.titleSize
        font.weight: Font.DemiBold
      }
    }
    Rectangle {
      width: parent.width
      height: 288
      radius: Theme.heroRadius
      color: Theme.primary
      Column {
        x: Theme.cardPadding
        y: Theme.cardPadding
        width: parent.width - Theme.cardPadding * 2
        spacing: Theme.space
        AppText {
          text: '查找附近充电站'
          width: parent.width
          wrapMode: Text.WordWrap
          font.pixelSize: Theme.headlineSize
          font.weight: Font.DemiBold
          color: 'white'
        }
      }
      HeroIllustration {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.cardPadding
        height: 172
      }
    }
    Column {
      width: parent.width
      spacing: Theme.space
      AppText {
        text: '手机号登录'
        font.pixelSize: Theme.titleSize
        font.weight: Font.DemiBold
      }
      AppText {
        width: parent.width
        text: '首次登录自动注册'
        color: Theme.muted
        wrapMode: Text.WordWrap
      }
    }
    Column {
      width: parent.width
      spacing: Theme.cardPadding
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
        text: mobile.busy ? '正在登录…' : '登录 / 注册'
        enabled: !mobile.busy
        onClicked: mobile.login(phone.text)
      }
    }
  }
}
