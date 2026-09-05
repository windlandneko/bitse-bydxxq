import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
  id: root
  objectName: 'mobileRoot'
  width: 430
  height: 860
  color: Theme.paper
  readonly property bool isTab: mobile.page === 'home' || mobile.page === 'orders' || mobile.page === 'profile'
  readonly property var pageInfo: ({
      "login": {
        "title": '',
        "source": 'LoginPage.qml'
      },
      "home": {
        "title": '附近电站',
        "source": 'HomePage.qml'
      },
      "orders": {
        "title": '我的订单',
        "source": 'OrdersPage.qml'
      },
      "profile": {
        "title": '我的',
        "source": 'ProfilePage.qml'
      },
      "station": {
        "title": '电站详情',
        "source": 'StationPage.qml'
      },
      "charge": {
        "title": '充电服务',
        "source": 'ChargingPage.qml'
      },
      "settlement": {
        "title": '订单结算',
        "source": 'ChargingPage.qml'
      },
      "receipt": {
        "title": '充电小票',
        "source": 'ReceiptPage.qml'
      },
      "location": {
        "title": '选择当前位置',
        "source": 'LocationPage.qml'
      },
      "recharge": {
        "title": '钱包充值',
        "source": 'RechargePage.qml'
      },
      "editProfile": {
        "title": '个人信息',
        "source": 'EditProfilePage.qml'
      }
    })[mobile.page]

  ColumnLayout {
    anchors.fill: parent
    spacing: 0
    Item {
      objectName: 'mobileTopBar'
      Layout.fillWidth: true
      Layout.preferredHeight: visible ? Theme.topBarHeight : 0
      visible: mobile.page !== 'login'
      RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.pagePadding
        anchors.rightMargin: Theme.pagePadding
        spacing: Theme.space
        IconButton {
          objectName: 'backButton'
          visible: !root.isTab
          iconName: 'arrow-left'
          label: '返回上一页'
          enabled: !mobile.busy
          onClicked: mobile.back()
        }
        AppText {
          Layout.fillWidth: true
          text: root.pageInfo.title
          font.pixelSize: Theme.titleSize
          font.weight: Font.DemiBold
          elide: Text.ElideRight
        }
        IconButton {
          objectName: 'refreshButton'
          visible: root.isTab || mobile.page === 'station'
          iconName: 'refresh-cw'
          label: '刷新当前页面'
          enabled: !mobile.busy
          onClicked: mobile.refresh()
        }
      }
    }
    Rectangle {
      Layout.fillWidth: true
      Layout.preferredHeight: mobile.busy ? 4 : 0
      visible: mobile.busy
      color: Theme.primaryLight
      clip: true
      Rectangle {
        width: parent.width * 0.3
        height: parent.height
        color: Theme.primary
        SequentialAnimation on x  {
          loops: Animation.Infinite
          running: mobile.busy
          NumberAnimation {
            from: -130
            to: root.width
            duration: 1000
            easing.type: Easing.InOutQuad
          }
        }
      }
    }
    Button {
      Layout.fillWidth: true
      Layout.preferredHeight: visible ? Math.max(48, offlineText.implicitHeight + 32) : 0
      visible: !mobile.online
      padding: Theme.pagePadding
      Accessible.name: '连接中断，点击重试'
      onClicked: mobile.refresh()
      background: Rectangle {
        color: parent.down ? '#f4e5c5' : '#fff2d9'
      }
      contentItem: AppText {
        id: offlineText
        text: '连接中断，点击重试'
        font.pixelSize: Theme.bodySize
        color: Theme.amber
        wrapMode: Text.WordWrap
      }
    }
    Rectangle {
      Layout.fillWidth: true
      Layout.preferredHeight: visible ? Math.max(64, errorText.implicitHeight + 32) : 0
      visible: mobile.error.length > 0
      color: Theme.dangerLight
      RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.pagePadding
        anchors.rightMargin: Theme.pagePadding
        spacing: Theme.space
        AppText {
          id: errorText
          Layout.fillWidth: true
          text: mobile.error
          wrapMode: Text.WordWrap
          color: Theme.danger
        }
        IconButton {
          label: '关闭错误提示'
          iconName: 'x'
          onClicked: mobile.clearError()
        }
      }
    }
    Loader {
      id: pageLoader
      objectName: 'pageLoader'
      Layout.fillWidth: true
      Layout.fillHeight: true
      source: root.pageInfo.source
    }
    Rectangle {
      objectName: 'mobileBottomNav'
      Layout.fillWidth: true
      Layout.preferredHeight: visible ? Theme.bottomNavHeight : 0
      visible: root.isTab
      color: Theme.card
      Rectangle {
        width: parent.width
        height: 1
        color: Theme.border
      }
      Row {
        anchors.fill: parent
        Repeater {
          model: [{
              "key": 'home',
              "label": '找电站',
              "icon": 'house'
            }, {
              "key": 'orders',
              "label": '订单',
              "icon": 'list'
            }, {
              "key": 'profile',
              "label": '我的',
              "icon": 'user'
            }]
          delegate: Button {
            id: tabButton
            required property var modelData
            width: root.width / 3
            height: Theme.bottomNavHeight
            objectName: 'tab_' + modelData.key
            padding: 0
            Accessible.name: modelData.label
            enabled: !mobile.busy
            highlighted: mobile.tab === modelData.key
            Accessible.role: Accessible.PageTab
            Accessible.selected: highlighted
            onClicked: mobile.selectTab(modelData.key)
            background: Rectangle {
              color: tabButton.down ? Theme.primaryLight : 'transparent'
              border.color: Theme.primary
              border.width: tabButton.visualFocus ? 2 : 0
            }
            contentItem: Column {
              opacity: tabButton.enabled ? 1 : 0.5
              spacing: Theme.microSpace
              topPadding: Theme.space
              Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 64
                height: 32
                radius: 16
                color: tabButton.highlighted ? Theme.accent : 'transparent'
                AppIcon {
                  anchors.centerIn: parent
                  name: tabButton.modelData.icon
                  opacity: tabButton.highlighted ? 1 : 0.65
                }
              }
              AppText {
                width: parent.width
                text: tabButton.modelData.label
                font.pixelSize: Theme.labelSize
                font.weight: tabButton.highlighted ? Font.DemiBold : Font.Normal
                color: tabButton.highlighted ? Theme.primary : Theme.muted
                horizontalAlignment: Text.AlignHCenter
              }
            }
          }
        }
      }
    }
  }
  Popup {
    id: toast
    objectName: 'toastPopup'
    x: Theme.pagePadding
    y: Math.max(Theme.space, root.height - height - 104)
    width: root.width - Theme.pagePadding * 2
    padding: Theme.cardPadding
    closePolicy: Popup.NoAutoClose
    property string message: ''
    background: Rectangle {
      color: '#ed223f30'
      radius: Theme.cardRadius
    }
    contentItem: AppText {
      text: toast.message
      color: 'white'
      wrapMode: Text.WordWrap
      horizontalAlignment: Text.AlignHCenter
    }
    Timer {
      id: toastTimer
      interval: 3500
      onTriggered: toast.close()
    }
  }
  Popup {
    id: unfinished
    objectName: 'unfinishedOrderPopup'
    anchors.centerIn: parent
    width: root.width - Theme.pagePadding * 2
    padding: Theme.cardPadding
    modal: true
    closePolicy: Popup.NoAutoClose
    property string message: ''
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
        text: '订单未完成'
        font.pixelSize: Theme.titleSize
        font.weight: Font.DemiBold
        wrapMode: Text.WordWrap
      }
      AppText {
        width: parent.width
        text: unfinished.message
        wrapMode: Text.WordWrap
        color: Theme.muted
        lineHeight: 1.5
      }
      ActionButton {
        objectName: 'unfinishedOrderContinue'
        width: parent.width
        text: '前往结算'
        onClicked: unfinished.close()
      }
    }
  }
  Connections {
    target: mobile
    function onNotification(message) {
      toast.message = message
      toast.open()
      toastTimer.restart()
    }
    function onUnfinishedOrder(message) {
      toast.close()
      unfinished.message = message
      unfinished.open()
    }
  }
}
