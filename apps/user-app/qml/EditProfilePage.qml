import QtQuick 2.15
import QtQuick.Controls 2.15

Flickable {
  objectName: 'editProfilePage'
  contentWidth: width
  contentHeight: content.height + 32
  boundsBehavior: Flickable.StopAtBounds
  clip: true
  Column {
    id: content
    x: 22
    y: 20
    width: parent.width - 44
    spacing: 27
    Column {
      width: parent.width
      spacing: 14
      Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        width: 98
        height: 98
        radius: 33
        color: '#e5e9e2'
        clip: true
        Image {
          anchors.centerIn: parent
          width: mobile.avatarSource ? 98 : 42
          height: width
          source: mobile.avatarSource || 'qrc:/icons/user.svg'
          fillMode: Image.PreserveAspectCrop
          opacity: mobile.avatarSource ? 1 : 0.4
        }
        MouseArea {
          objectName: 'chooseAvatarButton'
          anchors.fill: parent
          onClicked: mobile.chooseAvatar()
        }
      }
      ActionButton {
        anchors.horizontalCenter: parent.horizontalCenter
        height: 32
        text: '更换头像'
        tone: 'quiet'
        font.pixelSize: 12
        enabled: !mobile.busy
        onClicked: mobile.chooseAvatar()
      }
      AppText {
        width: parent.width
        text: 'PNG / JPEG，最大 2 MB'
        horizontalAlignment: Text.AlignHCenter
        color: Theme.muted
        font.pixelSize: 10
      }
    }
    Column {
      width: parent.width
      spacing: 11
      AppText {
        text: '昵称'
        font.pixelSize: 14
        font.weight: Font.DemiBold
      }
      AppField {
        id: nickname
        objectName: 'nicknameInput'
        width: parent.width
        Component.onCompleted: text = mobile.user.nickname || ''
        maximumLength: 24
        placeholderText: '给自己起个好听的名字'
        onAccepted: mobile.updateNickname(text)
      }
      AppText {
        text: '1 至 24 个字符'
        font.pixelSize: 11
        color: Theme.muted
      }
    }
    Column {
      width: parent.width
      spacing: 11
      AppText {
        text: '手机号'
        font.pixelSize: 14
        font.weight: Font.DemiBold
      }
      AppField {
        width: parent.width
        text: mobile.user.phone || ''
        readOnly: true
        color: Theme.muted
      }
      AppText {
        text: '手机号用于登录，暂不支持修改'
        font.pixelSize: 11
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
