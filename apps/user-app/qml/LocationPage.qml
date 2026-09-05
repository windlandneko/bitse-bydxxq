import QtQuick 2.15
import QtQuick.Controls 2.15

Flickable {
  objectName: 'locationPage'
  contentWidth: width
  contentHeight: content.height + 30
  boundsBehavior: Flickable.StopAtBounds
  clip: true
  Column {
    id: content
    x: 22
    y: 12
    width: parent.width - 44
    spacing: 21
    Column {
      width: parent.width
      spacing: 8
      AppText {
        text: '从哪里出发？'
        font.pixelSize: 25
        font.weight: Font.Bold
      }
      AppText {
        width: parent.width
        text: '选择常用区域，或输入地址定位附近电站。'
        font.pixelSize: 12
        color: Theme.muted
        wrapMode: Text.WordWrap
      }
    }
    Rectangle {
      width: parent.width
      height: currentLocation.height + 30
      radius: 18
      color: Theme.primaryLight
      Column {
        id: currentLocation
        x: 16
        y: 15
        width: parent.width - 32
        spacing: 8
        AppText {
          text: '当前位置'
          font.pixelSize: 11
          color: '#7d977d'
        }
        AppText {
          text: mobile.locationName
          width: parent.width
          font.weight: Font.Bold
          wrapMode: Text.WordWrap
          color: Theme.primary
        }
      }
    }
    Column {
      width: parent.width
      spacing: 12
      AppText {
        text: '输入具体地址'
        font.pixelSize: 15
        font.weight: Font.Bold
      }
      AppField {
        id: address
        objectName: 'geocodeAddressInput'
        width: parent.width
        placeholderText: '如：上海市人民广场'
        maximumLength: 150
        onAccepted: mobile.geocode(text)
      }
      ActionButton {
        objectName: 'geocodeButton'
        width: parent.width
        text: '定位到这里'
        enabled: !mobile.busy
        onClicked: mobile.geocode(address.text)
      }
    }
    Column {
      width: parent.width
      spacing: 12
      AppText {
        text: '常用区域'
        font.pixelSize: 17
        font.weight: Font.Bold
      }
      ComboBox {
        id: regionPicker
        objectName: 'locationPresetCombo'
        width: parent.width
        height: 54
        model: mobile.presets
        textRole: 'name'
        displayText: '下拉选择区域'
        font.pixelSize: 14
        background: Rectangle {
          color: 'white'
          radius: 14
          border.color: Theme.border
        }
        onActivated: {
          mobile.chooseLocation(currentIndex)
          mobile.back()
        }
      }
      Flow {
        width: parent.width
        spacing: 9
        Repeater {
          model: mobile.presets
          delegate: ActionButton {
            required property var modelData
            required property int index
            objectName: 'locationPreset_' + index
            text: modelData.name
            width: (content.width - 9) / 2
            height: 46
            horizontalPadding: 7
            font.pixelSize: 12
            tone: mobile.locationName === modelData.name ? 'primary' : 'secondary'
            onClicked: {
              mobile.chooseLocation(index)
              mobile.back()
            }
          }
        }
      }
    }
    AppText {
      width: parent.width
      text: '地址由腾讯位置服务解析；常用区域用于模拟当前位置。电站距离为两点直线距离，实际路线请查看导航。'
      color: Theme.muted
      font.pixelSize: 11
      wrapMode: Text.WordWrap
      lineHeight: 1.6
    }
  }
}
