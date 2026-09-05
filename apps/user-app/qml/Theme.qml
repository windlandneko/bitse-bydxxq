pragma Singleton
import QtQuick 2.15

QtObject {
  readonly property color paper: '#f6f7f3'
  readonly property color card: '#ffffff'
  readonly property color ink: '#192e24'
  readonly property color muted: '#65746a'
  readonly property color primary: '#245b43'
  readonly property color primaryPressed: '#174a33'
  readonly property color primaryLight: '#e5efe7'
  readonly property color accent: '#d9ee89'
  readonly property color border: '#e0e7de'
  readonly property color amber: '#8b601f'
  readonly property color danger: '#a84239'
  readonly property color dangerLight: '#fff0ec'
  readonly property color disabled: '#e2e7df'
  readonly property color disabledText: '#879286'

  readonly property int pagePadding: 16
  readonly property int cardPadding: 16
  readonly property int space: 8
  readonly property int sectionSpace: 24
  readonly property int controlGap: 12
  readonly property int microSpace: 4
  readonly property int iconSize: 24
  readonly property int touchSize: 48
  readonly property int topBarHeight: 64
  readonly property int bottomNavHeight: 80
  readonly property int cardRadius: 16
  readonly property int heroRadius: 24
  readonly property int bodySize: 14
  readonly property int bodyLargeSize: 16
  readonly property int labelSize: 12
  readonly property int titleSize: 22
  readonly property int headlineSize: 28
}
