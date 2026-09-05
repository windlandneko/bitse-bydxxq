import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Flickable {
  id: screen
  objectName: 'profilePage'
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
    Button {
      id: profileButton
      objectName: 'editProfileButton'
      width: parent.width
      height: 96
      padding: Theme.cardPadding
      Accessible.name: '修改个人信息，' + (mobile.user.nickname || '')
      onClicked: mobile.navigate('editProfile')
      background: Rectangle {
        radius: Theme.cardRadius
        color: profileButton.down ? Theme.primaryLight : Theme.card
        border.color: Theme.primary
        border.width: profileButton.visualFocus ? 2 : 0
      }
      contentItem: RowLayout {
        spacing: Theme.cardPadding
        Rectangle {
          Layout.preferredWidth: 64
          Layout.preferredHeight: 64
          radius: Theme.heroRadius
          color: '#e6eae3'
          Image {
            anchors.centerIn: parent
            width: mobile.avatarSource ? 48 : 24
            height: width
            source: mobile.avatarSource || 'qrc:/icons/user.svg'
            fillMode: Image.PreserveAspectCrop
            opacity: mobile.avatarSource ? 1 : 0.65
          }
        }
        Column {
          Layout.fillWidth: true
          spacing: Theme.microSpace
          AppText {
            width: parent.width
            text: mobile.user.nickname || ''
            font.pixelSize: Theme.titleSize
            font.weight: Font.DemiBold
            elide: Text.ElideRight
          }
          AppText {
            text: mobile.user.phone || ''
            color: Theme.muted
          }
        }
        AppIcon {
          name: 'chevron-right'
          Layout.preferredWidth: 24
          Layout.preferredHeight: 24
        }
      }
    }
    Rectangle {
      width: parent.width
      height: walletContent.height + Theme.cardPadding * 2
      radius: Theme.heroRadius
      color: Theme.primary
      Column {
        id: walletContent
        x: Theme.cardPadding
        y: Theme.cardPadding
        width: parent.width - Theme.cardPadding * 2
        spacing: Theme.cardPadding
        AppText {
          text: '钱包余额'
          color: '#d2e2d5'
        }
        MoneyText {
          cents: Number(mobile.user.balanceCents || 0)
          valueColor: 'white'
          valueSize: 36
        }
        ActionButton {
          objectName: 'walletRechargeButton'
          width: parent.width
          text: '充值'
          leadingIcon: 'plus'
          tone: 'secondary'
          onClicked: mobile.navigate('recharge')
        }
      }
    }
    Rectangle {
      width: parent.width
      height: frozenText.implicitHeight + Theme.cardPadding * 2
      visible: mobile.user.status === 'frozen'
      radius: Theme.cardRadius
      color: Theme.dangerLight
      AppText {
        id: frozenText
        x: Theme.cardPadding
        y: Theme.cardPadding
        width: parent.width - Theme.cardPadding * 2
        text: '账号已冻结，暂时无法预约或开始充电。已有订单仍可结束、结算，请联系管理员。'
        wrapMode: Text.WordWrap
        color: Theme.danger
        lineHeight: 1.5
      }
    }
    Column {
      width: parent.width
      spacing: Theme.space
      MenuRow {
        width: parent.width
        title: '个人信息'
        description: '修改头像与昵称'
        iconName: 'user'
        onClicked: mobile.navigate('editProfile')
      }
      MenuRow {
        width: parent.width
        title: '充电记录'
        description: '订单与充电小票'
        iconName: 'list'
        onClicked: mobile.selectTab('orders')
      }
      MenuRow {
        width: parent.width
        title: '当前位置'
        description: mobile.locationName
        iconName: 'map-pin'
        onClicked: mobile.navigate('location')
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
  }
  Popup {
    id: logoutConfirm
    anchors.centerIn: Overlay.overlay
    width: screen.width - Theme.pagePadding * 2
    padding: Theme.cardPadding
    modal: true
    background: Rectangle {
      color: Theme.card
      radius: Theme.heroRadius
    }
    Overlay.modal: Rectangle {
      color: '#75102017'
    }
    contentItem: Column {
      spacing: Theme.cardPadding
      AppText {
        width: parent.width
        text: '退出当前账号？'
        font.pixelSize: Theme.titleSize
        font.weight: Font.DemiBold
        wrapMode: Text.WordWrap
      }
      AppText {
        width: parent.width
        text: Number(mobile.activeOrder.id || 0) > 0 ? '你还有未完成订单。退出不会停止充电或取消预约，再次登录可继续处理。' : '订单和余额会保留，下次输入手机号即可登录。'
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
        text: '暂不退出'
        tone: 'quiet'
        onClicked: logoutConfirm.close()
      }
    }
  }
}
