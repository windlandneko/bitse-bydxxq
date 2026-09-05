import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
  id: root
  objectName: 'mobileRoot'
  width: 430
  height: 860
  color: Theme.paper
  property bool isTab: mobile.page === 'home' || mobile.page === 'orders' || mobile.page === 'profile'
  property string title: ({
      "home": '发现好电站',
      "orders": '我的订单',
      "profile": '我的',
      "station": '电站详情',
      "charge": '充电服务',
      "settlement": '订单结算',
      "receipt": '充电小票',
      "location": '选择当前位置',
      "recharge": '钱包充值',
      "editProfile": '个人信息'
    })[mobile.page] || ''
  function money(value) {
    return (Number(value || 0) / 100).toFixed(2)
  }

  ColumnLayout {
    anchors.fill: parent
    spacing: 0

    Item {
      Layout.fillWidth: true
      Layout.preferredHeight: mobile.page === 'login' ? 0 : 78
      visible: mobile.page !== 'login'
      ActionButton {
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        width: 44
        height: 44
        text: '‹'
        font.pixelSize: 32
        tone: 'quiet'
        visible: !root.isTab
        objectName: 'backButton'
        onClicked: mobile.back()
      }
      Column {
        anchors.left: parent.left
        anchors.leftMargin: root.isTab ? 24 : 68
        anchors.verticalCenter: parent.verticalCenter
        spacing: 4
        AppText {
          text: root.title
          font.pixelSize: root.isTab ? 25 : 20
          font.weight: Font.Bold
        }
        AppText {
          visible: root.isTab
          text: mobile.page === 'home' ? '每一程，都电力满满' : mobile.page === 'orders' ? '每一次补能，都有记录' : '与好状态，一路同行'
          color: Theme.muted
          font.pixelSize: 12
        }
      }
      ToolButton {
        anchors.right: parent.right
        anchors.rightMargin: 18
        anchors.verticalCenter: parent.verticalCenter
        width: 42
        height: 42
        visible: root.isTab || mobile.page === 'station'
        objectName: 'refreshButton'
        enabled: !mobile.busy
        onClicked: mobile.refresh()
        contentItem: Image {
          source: 'qrc:/icons/refresh-cw.svg'
          sourceSize.width: 20
          sourceSize.height: 20
          fillMode: Image.Pad
          opacity: 0.65
        }
        background: Rectangle {
          radius: 15
          color: 'white'
        }
      }
    }

    Rectangle {
      Layout.fillWidth: true
      Layout.preferredHeight: mobile.busy ? 3 : 0
      visible: mobile.busy
      color: Theme.primaryLight
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

    Rectangle {
      Layout.fillWidth: true
      Layout.preferredHeight: visible ? offlineText.implicitHeight + 22 : 0
      visible: !mobile.online
      color: '#fff2d9'
      AppText {
        id: offlineText
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 22
        anchors.verticalCenter: parent.verticalCenter
        text: '暂时无法连接服务，正在重新连接。点击此处重试'
        font.pixelSize: 12
        color: Theme.amber
        wrapMode: Text.WordWrap
      }
      MouseArea {
        anchors.fill: parent
        onClicked: mobile.refresh()
      }
    }

    Rectangle {
      Layout.fillWidth: true
      Layout.preferredHeight: visible ? errorText.implicitHeight + 28 : 0
      visible: mobile.error.length > 0
      color: Theme.dangerLight
      AppText {
        id: errorText
        anchors.left: parent.left
        anchors.right: dismissError.left
        anchors.leftMargin: 22
        anchors.verticalCenter: parent.verticalCenter
        text: mobile.error
        font.pixelSize: 13
        wrapMode: Text.WordWrap
        color: Theme.danger
      }
      ToolButton {
        id: dismissError
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        text: '×'
        onClicked: mobile.clearError()
      }
    }

    Loader {
      id: pageLoader
      objectName: 'pageLoader'
      Layout.fillWidth: true
      Layout.fillHeight: true
      source: ({
          "login": 'LoginPage.qml',
          "home": 'HomePage.qml',
          "station": 'StationPage.qml',
          "charge": 'ChargingPage.qml',
          "settlement": 'ChargingPage.qml',
          "receipt": 'ReceiptPage.qml',
          "orders": 'OrdersPage.qml',
          "profile": 'ProfilePage.qml',
          "location": 'LocationPage.qml',
          "recharge": 'RechargePage.qml',
          "editProfile": 'EditProfilePage.qml'
        })[mobile.page] || 'HomePage.qml'
    }

    Rectangle {
      Layout.fillWidth: true
      Layout.preferredHeight: root.isTab ? 82 : 0
      visible: root.isTab
      color: 'white'
      Rectangle {
        width: parent.width
        height: 1
        color: Theme.border
      }
      Row {
        anchors.fill: parent
        anchors.topMargin: 9
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
          delegate: Item {
            required property var modelData
            width: root.width / 3
            height: 66
            objectName: 'tab_' + modelData.key
            Rectangle {
              anchors.horizontalCenter: parent.horizontalCenter
              y: 0
              width: 58
              height: 32
              radius: 13
              color: mobile.tab === modelData.key ? Theme.accent : 'transparent'
              Image {
                anchors.centerIn: parent
                source: 'qrc:/icons/' + modelData.icon + '.svg'
                width: 20
                height: 20
                opacity: mobile.tab === modelData.key ? 0.9 : 0.4
              }
            }
            AppText {
              anchors.horizontalCenter: parent.horizontalCenter
              y: 40
              text: modelData.label
              font.pixelSize: 11
              font.weight: mobile.tab === modelData.key ? Font.Bold : Font.Normal
              color: mobile.tab === modelData.key ? Theme.primary : Theme.muted
            }
            MouseArea {
              anchors.fill: parent
              onClicked: mobile.selectTab(modelData.key)
            }
          }
        }
      }
    }
  }

  Popup {
    id: toast
    objectName: 'toastPopup'
    x: 22
    y: Math.max(20, root.height - height - (root.isTab ? 100 : 110))
    width: root.width - 44
    padding: 17
    closePolicy: Popup.NoAutoClose
    property string message: ''
    background: Rectangle {
      color: '#e6223f30'
      radius: 17
    }
    contentItem: AppText {
      text: toast.message
      color: 'white'
      font.pixelSize: 13
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
    width: root.width - 52
    padding: 24
    modal: true
    closePolicy: Popup.NoAutoClose
    property string message: ''
    background: Rectangle {
      color: Theme.card
      radius: 25
    }
    Overlay.modal: Rectangle {
      color: '#75102017'
    }
    contentItem: Column {
      spacing: 20
      AppText {
        text: '有一笔订单等您处理'
        font.pixelSize: 20
        font.weight: Font.Bold
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
