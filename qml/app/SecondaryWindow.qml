import QtQuick 2.15
import QtQuick.Controls 2.15
import scene2d
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

    // ── Plot2D (dedicated indx=6) ──
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

            onSettingsClicked: if (root.store) root.store.toggleEchogramSettings(this, qsTr("Second window"), root.store.secondaryEchogramKey)

            Component.onCompleted: {
                setIndx(6)
                if (typeof core !== "undefined" && core && typeof core.registerPlot2D === "function")
                    core.registerPlot2D(this)
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#FFFFFF"
        opacity: (root.mode !== "" && root.store
                  && root.store.highlightedLeafId === root.store.secondaryEchogramKey) ? 0.16 : 0.0
        visible: opacity > 0
        z: 50
        Behavior on opacity { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
        readonly property int _focus: root.store ? root.store.settingsFocusLeafId : -1
        opacity: (_focus !== -1 && root.store && _focus !== root.store.secondaryEchogramKey) ? 0.55 : 0.0
        visible: opacity > 0.001
        z: 51
        Behavior on opacity { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
    }


    Loader {
        id: videoLoader
        anchors.fill: parent
        visible: root.mode === "Video"
        active: root.mode === "Video"
        sourceComponent: videoComponent
    }

    Component {
        id: videoComponent

        VideoSurface {
            store: root.store
            contentId: root.store ? root.store.secondaryVideoContentId : ""
        }
    }

    Rectangle {
        id: modePicker
        anchors.fill: parent
        visible: root.mode === ""
        z: ZOrder.inputLockOverlay - 1
        color: "#020617D9"

        MouseArea {
            anchors.fill: parent
        }

        Column {
            anchors.centerIn: parent
            spacing: Math.round(12 * AppPalette.scale)

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Choose pane type")
                color: AppPalette.text
                font.pixelSize: Math.round(18 * AppPalette.scale)
                font.bold: true
            }

            Row {
                id: secondaryTypeRow
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: Math.round(10 * AppPalette.scale)

                readonly property int cellWidth: Math.max(Math.round(96 * AppPalette.scale),
                                                          secondary2DButton.implicitWidth,
                                                          secondaryVideoButton.implicitWidth)
                readonly property int cellHeight: Math.max(Math.round(46 * AppPalette.scale),
                                                           secondary2DButton.implicitHeight,
                                                           secondaryVideoButton.implicitHeight)

                KButton {
                    id: secondary2DButton
                    readonly property bool canChoose2D: root.store ? root.store.canSecondaryWindowChoose2D() : false
                    text: qsTr("2D")
                    width: secondaryTypeRow.cellWidth
                    height: secondaryTypeRow.cellHeight
                    enabled: canChoose2D
                    opacity: enabled ? 1.0 : 0.45
                    onClicked: if (root.store) root.store.setSecondaryWindowMode("2D")
                }

                KButton {
                    id: secondaryVideoButton
                    text: qsTr("Video")
                    width: secondaryTypeRow.cellWidth
                    height: secondaryTypeRow.cellHeight
                    onClicked: if (root.store) root.store.setSecondaryWindowMode("Video")
                }
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: !secondary2DButton.canChoose2D
                text: qsTr("Echogram limit reached")
                color: "#C7D2FE"
                font.pixelSize: Math.round(12 * AppPalette.scale)
            }
        }
    }

    FileOpeningOverlay { }

    MouseArea {
        anchors.fill: parent
        z: ZOrder.inputLockOverlay
        visible: !!(root.store && root.store.inputLocked)
        enabled: visible
        acceptedButtons: Qt.AllButtons
        hoverEnabled: true
        preventStealing: true
        propagateComposedEvents: false
        onPressed:       function(mouse) { mouse.accepted = true }
        onReleased:      function(mouse) { mouse.accepted = true }
        onClicked:       function(mouse) { mouse.accepted = true }
        onDoubleClicked: function(mouse) { mouse.accepted = true }
        onPressAndHold:  function(mouse) { mouse.accepted = true }
        onWheel:         function(wheel) { wheel.accepted = true }
    }
}
