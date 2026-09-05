import QtQuick 2.15

Image {
  property string name: ''
  width: Theme.iconSize
  height: Theme.iconSize
  source: name ? 'qrc:/icons/' + name + '.svg' : ''
  sourceSize.width: width * 2
  sourceSize.height: height * 2
  fillMode: Image.PreserveAspectFit
  Accessible.ignored: true
}
