import QtQuick 2.15
import kqml_types 1.0

Column {
    id: page

    required property var store

    readonly property bool touch: (typeof inputDeviceTracker !== "undefined" && inputDeviceTracker)
                                  ? inputDeviceTracker.touchMode : false

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

    KIsland {
        title: qsTr("Workspace")

        KIslandRow {
            label: qsTr("Maximise or restore a pane")
            caption: page.touch ? qsTr("Double-tap the pane — 2D, 3D, video, popups")
                                : qsTr("Double-click the pane — 2D, 3D, video, popups")
        }

        KIslandRow {
            label: qsTr("Reset a splitter to the middle")
            caption: page.touch ? qsTr("Double-tap the splitter between panes")
                                : qsTr("Double-click the splitter between panes")
        }
    }

    KIsland {
        title: qsTr("2D echogram")

        KIslandRow {
            label: qsTr("Pick a sounding")
            caption: page.touch ? qsTr("Drag one finger — shows the loupe and the sounding details")
                                : qsTr("Drag with the left button — shows the loupe and the sounding details")
        }

        KIslandRow {
            label: qsTr("Place a point of interest")
            caption: page.touch ? qsTr("Press and hold — opens the context menu")
                                : qsTr("Right-click — opens the context menu")
        }

        KIslandRow {
            label: qsTr("Scroll the echogram")
            caption: page.touch ? qsTr("Horizontal pinch") : qsTr("Wheel")
        }

        KIslandRow {
            label: qsTr("Move the echogram up and down")
            caption: page.touch ? qsTr("Vertical pinch") : qsTr("Shift + wheel")
        }

        KIslandRow {
            label: qsTr("Change the vertical scale")
            caption: page.touch ? qsTr("Stretch the pinch vertically") : qsTr("Ctrl + wheel")
        }
    }

    KIsland {
        title: qsTr("3D scene")

        KIslandRow {
            label: qsTr("Move the camera")
            caption: page.touch ? qsTr("Drag one finger") : qsTr("Drag with the left button")
        }

        KIslandRow {
            label: qsTr("Rotate the camera")
            caption: page.touch ? qsTr("Rotate the pinch") : qsTr("Ctrl + drag sideways")
        }

        KIslandRow {
            label: qsTr("Tilt the camera")
            caption: page.touch ? qsTr("Vertical pinch") : qsTr("Ctrl + drag up or down")
        }
    }
}
