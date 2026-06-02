import QtQuick 2.15
import QtQuick.Controls 2.15
import "../scene2d"
import kqml_types 1.0

Item {
    id: root

    required property var store

    readonly property string mode: store ? store.secondaryWindowMode : ""

    // Forwarded to MainWindow.handleHotkeyKeyEvent so all hotkeys work in second window.
    signal hotkeyReceived(var event)

    // Exposed for ESC close in MainWindow.
    readonly property var plot2DInstance: plot2DLoader.item

    focus: true
    Keys.onReleased: function(event) {
        root.hotkeyReceived(event)
    }

    InputDeviceState {
        id: inputStateObject
    }

    Rectangle {
        anchors.fill: parent
        color: AppPalette.bg
    }

    // ── Plot2D (dedicated indx=6, settings persisted under "Plot2D_6") ──
    // Smooth resize on window fullscreen toggle — mirrors PaneFrame's animation.
    Loader {
        id: plot2DLoader
        x: 0
        y: 0
        width: parent.width
        height: parent.height
        visible: root.mode === "2D"
        active: root.mode === "2D"
        sourceComponent: plot2DComponent

        Behavior on width  { NumberAnimation { duration: 220; easing.type: Easing.InOutCubic } }
        Behavior on height { NumberAnimation { duration: 220; easing.type: Easing.InOutCubic } }
    }

    Component {
        id: plot2DComponent
        Plot2D {
            indx: 6
            inputState: inputStateObject
            externalInputRouting: false
            is3dVisible: false

            onSettingsClicked: if (root.store) root.store.openEchogramSettings(this, qsTr("Second window"))

            Component.onCompleted: {
                setIndx(6)
                if (typeof core !== "undefined" && core && typeof core.registerPlot2D === "function")
                    core.registerPlot2D(this)
            }
        }
    }

    Rectangle {
        anchors.fill: plot2DLoader
        color: "#FFFFFF"
        opacity: (root.mode === "2D" && root.store
                  && root.store.highlightedLeafId === root.store.secondaryEchogramKey) ? 0.16 : 0.0
        visible: opacity > 0
        z: 50
        Behavior on opacity { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
    }

    // ── Empty state: shown only when 2D slot is unavailable (5-echogram limit). ──
    Text {
        anchors.centerIn: parent
        visible: root.mode === ""
        text: qsTr("Echogram limit reached")
        color: AppPalette.textSecond
        font.pixelSize: 16
    }

    FileOpeningOverlay { }
}
