import QtQuick 2.15
import QtQuick.Controls 2.15

Flickable {
  id: screen
  objectName: 'profilePage'
  contentWidth: width
  contentHeight: content.height + 28
  boundsBehavior: Flickable.StopAtBounds
  clip: true
  ScrollBar.vertical: ScrollBar {
    policy: ScrollBar.AsNeeded
  }
  Column {
    id: content
    x: 22
    y: 16
    width: parent.width - 44
    spacing: 22
    Item {
      width: parent.width
      height: 80
      Rectangle {
        id: avatar
        width: 72
        height: 72
        radius: 26
        color: '#e8ebe6'
        clip: true
        Image {
          anchors.centerIn: parent
          width: mobile.avatarSource ? 72 : 34
          height: width
          source: mobile.avatarSource || 'qrc:/icons/user.svg'
          fillMode: Image.PreserveAspectCrop
          opacity: mobile.avatarSource ? 1 : 0.4
        }
      }
      Column {
        anchors.left: avatar.right
        anchors.leftMargin: 16
        anchors.verticalCenter: avatar.verticalCenter
        width: parent.width - 116
        spacing: 8
        AppText {
          text: mobile.user.nickname || ''
          width: parent.width
          elide: Text.ElideRight
          font.pixelSize: 24
          font.weight: Font.Bold
        }
        AppText {
          text: mobile.user.phone || ''
          font.pixelSize: 13
          color: Theme.muted
        }
      }
      AppText {
        anchors.right: parent.right
        y: 23
        text: '›'
        color: Theme.muted
        font.pixelSize: 27
      }
      MouseArea {
        objectName: 'editProfileButton'
        anchors.fill: parent
        onClicked: mobile.navigate('editProfile')
      }
    }
    Rectangle {
      width: parent.width
      height: 171
      radius: 25
      color: Theme.primary
      clip: true
      Rectangle {
        x: parent.width - 155
        y: 15
        width: 140
        height: 140
        radius: 70
        color: '#306b4f'
      }
      Column {
        x: 23
        y: 23
        spacing: 13
        AppText {
          text: '我的钱包'
          color: '#c7dbc9'
          font.pixelSize: 12
        }
        AppText {
          text: '¥ ' + (Number(mobile.user.balanceCents || 0) / 100).toFixed(2)
          color: 'white'
          font.pixelSize: 36
          font.weight: Font.Bold
        }
        AppText {
          text: '余额随充随用，轻松开启下一程'
          color: '#a9c5b3'
          font.pixelSize: 10
        }
      }
      ActionButton {
        objectName: 'walletRechargeButton'
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 20
        width: 90
        height: 36
        text: '充值 +'
        tone: 'secondary'
        font.pixelSize: 13
        onClicked: mobile.navigate('recharge')
      }
    }
    Rectangle {
      width: parent.width
      height: frozenText.implicitHeight + 30
      visible: mobile.user.status === 'frozen'
      radius: 15
      color: Theme.dangerLight
      AppText {
        id: frozenText
        x: 15
        y: 15
        width: parent.width - 30
        text: '账号已冻结，暂时无法预约或开始新的充电。已有订单仍可结束并结算，请联系管理员处理。'
        wrapMode: Text.WordWrap
        color: Theme.danger
        font.pixelSize: 12
        lineHeight: 1.5
      }
    }
    Rectangle {
      width: parent.width
      height: menu.height
      radius: 22
      color: 'white'
      Column {
        id: menu
        width: parent.width
        Repeater {
          model: [{
              "label": '个人信息',
              "description": '修改头像与昵称',
              "icon": 'user',
              "target": 'editProfile'
            }, {
              "label": '充电记录',
              "description": '查看订单与充电小票',
              "icon": 'list',
              "target": 'orders'
            }, {
              "label": '当前位置',
              "description": mobile.locationName,
              "icon": 'navigation',
              "target": 'location'
            }]
          delegate: Item {
            required property var modelData
            required property int index
            width: menu.width
            height: 78
            Rectangle {
              x: 18
              y: 19
              width: 40
              height: 40
              radius: 13
              color: '#f0f3ed'
              Image {
                anchors.centerIn: parent
                width: 19
                height: 19
                source: 'qrc:/icons/' + modelData.icon + '.svg'
                opacity: 0.65
              }
            }
            Column {
              x: 72
              y: 21
              width: parent.width - 107
              spacing: 6
              AppText {
                text: modelData.label
                font.pixelSize: 14
                font.weight: Font.DemiBold
              }
              AppText {
                text: modelData.description
                width: parent.width
                elide: Text.ElideRight
                font.pixelSize: 10
                color: Theme.muted
              }
            }
            AppText {
              anchors.right: parent.right
              anchors.rightMargin: 20
              y: 22
              text: '›'
              color: '#a1aca2'
              font.pixelSize: 25
            }
            Rectangle {
              x: 72
              y: 77
              width: parent.width - 92
              height: 1
              color: Theme.border
              visible: index !== 2
            }
            MouseArea {
              anchors.fill: parent
              onClicked: modelData.target === 'orders' ? mobile.selectTab('orders') : mobile.navigate(modelData.target)
            }
          }
        }
      }
    }
    ActionButton {
      objectName: 'logoutButton'
      width: parent.width
      text: '退出登录'
      tone: 'quiet'
      enabled: !mobile.busy
      onClicked: logoutConfirm.open()
    }
    Column {
      width: parent.width
      spacing: 6
      AppText {
        width: parent.width
        text: '智充出行'
        font.pixelSize: 13
        font.weight: Font.Bold
        color: '#a0aaa1'
        horizontalAlignment: Text.AlignHCenter
      }
      AppText {
        width: parent.width
        text: '让绿色出行，成为日常'
        font.pixelSize: 10
        color: '#adb5ab'
        horizontalAlignment: Text.AlignHCenter
      }
    }
  }
  Popup {
    id: logoutConfirm
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
      spacing: 20
      AppText {
        text: '退出当前账号？'
        font.pixelSize: 22
        font.weight: Font.Bold
      }
      AppText {
        width: parent.width
        text: Number(mobile.activeOrder.id || 0) > 0 ? '你还有未完成订单。退出不会停止充电或取消预约，再次登录可继续处理。' : '再次输入手机号即可登录，订单和余额会为你保留。'
        color: Theme.muted
        wrapMode: Text.WordWrap
        lineHeight: 1.5
      }
      ActionButton {
        objectName: 'confirmLogoutButton'
        width: parent.width
        text: '确认退出'
        onClicked: {
          logoutConfirm.close()
          mobile.logout()
        }
      }
      ActionButton {
        width: parent.width
        text: '再看看'
        tone: 'quiet'
        onClicked: logoutConfirm.close()
      }
    }
  }
}
