import QtQuick 2.15
import QtQuick.Controls 2.15
import kqml_types 1.0

Flickable {
    id: scroll

    property real maxHeight: 0
    property bool active: true

    readonly property alias body: bodyItem
    readonly property real bodyWidth: bodyItem.width
    readonly property real bodyHeight: bodyItem.implicitHeight
    readonly property real viewHeight: maxHeight > 0 ? Math.min(bodyHeight, maxHeight) : bodyHeight
    readonly property bool overflowing: bodyHeight > viewHeight + 0.5

    width: bodyWidth
    height: viewHeight
    contentWidth: bodyWidth
    contentHeight: bodyHeight
    clip: true
    interactive: overflowing
    boundsBehavior: Flickable.StopAtBounds
    ScrollIndicator.vertical: ScrollIndicator {}

    onContentHeightChanged: Qt.callLater(returnToBounds)
    onHeightChanged: Qt.callLater(returnToBounds)

    ServoPanelBody {
        id: bodyItem
        active: scroll.active
    }
}
