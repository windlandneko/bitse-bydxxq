import QtQuick 2.15

Image {
  source: 'qrc:/assets/illustrations/charging-hero.svg'
  fillMode: Image.PreserveAspectFit
  sourceSize.width: Math.max(1, width * 2)
  sourceSize.height: Math.max(1, height * 2)
  Accessible.ignored: true
  onStatusChanged: {
    if (status === Image.Error && source.toString().endsWith('.svg'))
      source = 'qrc:/assets/illustrations/charging-hero.png'
  }
}
