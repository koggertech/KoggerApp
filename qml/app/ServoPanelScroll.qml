import QtQuick 2.15
import QtQuick.Controls 2.15
import kqml_types 1.0

// The servo body under a height cap. Below the cap the panel is exactly as tall as its content;
// above it the panel stops growing and the content scrolls, which is what opening "More
// settings" on a short window needs.
//
// `interactive` follows the overflow: a Flickable that never has anything to scroll still
// rubber-bands under a finger, and on a panel that floats over a chart that reads as breakage.
Flickable {
    id: scroll

    property real maxHeight: 0

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

    // Collapsing "More settings" while scrolled down leaves contentY past the new end, and
    // StopAtBounds only enforces bounds during a flick -- so the panel would hold a strip of
    // blank until the next touch. Deferred, because the height animates its way there.
    onContentHeightChanged: Qt.callLater(returnToBounds)
    onHeightChanged: Qt.callLater(returnToBounds)

    ServoPanelBody {
        id: bodyItem
    }
}
