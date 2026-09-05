import QtQuick 2.15
import QtQuick.Controls 2.15

Flickable {
  objectName: 'locationPage'
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
      height: currentLocation.height + Theme.cardPadding * 2
      radius: Theme.cardRadius
      color: Theme.primaryLight
      Column {
        id: currentLocation
        x: Theme.cardPadding
        y: Theme.cardPadding
        width: parent.width - Theme.cardPadding * 2
        spacing: Theme.space
        AppText {
          text: '当前位置'
          color: Theme.muted
          font.pixelSize: Theme.labelSize
        }
        AppText {
          width: parent.width
          text: mobile.locationName
          font.pixelSize: Theme.bodyLargeSize
          font.weight: Font.Medium
          color: Theme.primary
          wrapMode: Text.WordWrap
        }
      }
    }
    Column {
      width: parent.width
      spacing: Theme.cardPadding
      AppText {
        text: '输入地址'
        font.pixelSize: Theme.bodyLargeSize
        font.weight: Font.Medium
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
      spacing: Theme.cardPadding
      AppText {
        text: '常用区域'
        font.pixelSize: Theme.bodyLargeSize
        font.weight: Font.Medium
      }
      ComboBox {
        id: regionPicker
        objectName: 'locationPresetCombo'
        width: parent.width
        height: 56
        model: mobile.presets
        textRole: 'name'
        displayText: '下拉选择区域'
        leftPadding: Theme.cardPadding
        rightPadding: 56
        Accessible.name: '选择常用区域'
        font.pixelSize: Theme.bodySize
        contentItem: AppText {
          text: regionPicker.displayText
          verticalAlignment: Text.AlignVCenter
          elide: Text.ElideRight
        }
        indicator: AppIcon {
          name: 'chevron-down'
          anchors.right: parent.right
          anchors.rightMargin: Theme.cardPadding
          anchors.verticalCenter: parent.verticalCenter
        }
        background: Rectangle {
          color: regionPicker.down ? Theme.primaryLight : Theme.card
          radius: Theme.cardRadius
          border.color: regionPicker.visualFocus ? Theme.primary : Theme.border
          border.width: regionPicker.visualFocus ? 2 : 1
        }
        delegate: ItemDelegate {
          width: regionPicker.width
          height: 48
          text: modelData.name
          font.pixelSize: Theme.bodySize
          padding: Theme.cardPadding
          highlighted: regionPicker.highlightedIndex === index
        }
        onActivated: {
          mobile.chooseLocation(currentIndex)
          mobile.back()
        }
      }
      Flow {
        width: parent.width
        spacing: Theme.space
        Repeater {
          model: mobile.presets
          delegate: ActionButton {
            required property var modelData
            required property int index
            objectName: 'locationPreset_' + index
            text: modelData.name
            width: (content.width - Theme.space) / 2
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
      text: '地址由腾讯位置服务解析，区域选择用于模拟当前位置。列表显示直线距离，实际路线请查看导航。'
      color: Theme.muted
      font.pixelSize: Theme.labelSize
      wrapMode: Text.WordWrap
      lineHeight: 1.5
    }
  }
}
