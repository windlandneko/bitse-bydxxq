import QtQuick 2.15
import QtQuick.Controls 2.15

Flickable {
  objectName: 'editProfilePage'
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
    Column {
      width: parent.width
      spacing: Theme.space
      Button {
        id: avatarButton
        objectName: 'chooseAvatarButton'
        anchors.horizontalCenter: parent.horizontalCenter
        width: 96
        height: 96
        padding: 12
        enabled: !mobile.busy
        Accessible.name: '更换头像'
        onClicked: mobile.chooseAvatar()
        background: Rectangle {
          radius: Theme.heroRadius
          color: avatarButton.down ? '#d3e1d1' : '#e5e9e2'
          border.width: avatarButton.visualFocus ? 2 : 0
          border.color: Theme.primary
        }
        contentItem: Image {
          source: mobile.avatarSource || 'qrc:/icons/user.svg'
          sourceSize.width: 144
          sourceSize.height: 144
          fillMode: Image.PreserveAspectFit
          opacity: mobile.avatarSource ? 1 : 0.5
        }
      }
      ActionButton {
        anchors.horizontalCenter: parent.horizontalCenter
        text: '更换头像'
        tone: 'quiet'
        enabled: !mobile.busy
        onClicked: mobile.chooseAvatar()
      }
      AppText {
        width: parent.width
        text: 'PNG / JPEG，最大 2 MB'
        horizontalAlignment: Text.AlignHCenter
        color: Theme.muted
        font.pixelSize: Theme.labelSize
      }
    }
    Column {
      width: parent.width
      spacing: Theme.space
      AppText {
        text: '昵称'
        font.pixelSize: Theme.bodyLargeSize
        font.weight: Font.Medium
      }
      AppField {
        id: nickname
        objectName: 'nicknameInput'
        width: parent.width
        Component.onCompleted: text = mobile.user.nickname || ''
        maximumLength: 24
        placeholderText: '请输入昵称'
        Accessible.name: '昵称'
        onAccepted: mobile.updateNickname(text)
      }
      AppText {
        text: '1 至 24 个字符'
        font.pixelSize: Theme.labelSize
        color: Theme.muted
      }
    }
    Column {
      width: parent.width
      spacing: Theme.space
      AppText {
        text: '手机号'
        font.pixelSize: Theme.bodyLargeSize
        font.weight: Font.Medium
      }
      AppField {
        width: parent.width
        text: mobile.user.phone || ''
        readOnly: true
        color: Theme.muted
        Accessible.name: '登录手机号，不可修改'
      }
      AppText {
        text: '手机号用于登录，暂不支持修改'
        font.pixelSize: Theme.labelSize
        color: Theme.muted
      }
    }
    ActionButton {
      objectName: 'saveProfileButton'
      width: parent.width
      text: '保存修改'
      enabled: !mobile.busy
      onClicked: mobile.updateNickname(nickname.text)
    }
  }
}
