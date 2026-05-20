import QtQuick 2.15
import QtQuick.Controls 2.15
import "../scene2d"
import kqml_types 1.0

Item {
    id: root

    required property var store

    readonly property string mode: store ? store.secondaryWindowMode : ""
    readonly property bool canPick2D: store ? store.secondary2DAvailable : true
    // 3D in secondary is deferred: cross-window reparenting of QQuickFramebufferObject
    // breaks viewport/input. Needs C++ changes in GraphicsScene3dView.
    readonly property bool threeDSupported: false

    InputDeviceState {
        id: inputStateObject
    }

    Rectangle {
        anchors.fill: parent
        color: AppPalette.bg
    }

    // ── Mode picker (visible only while mode === "") ────────────────────────
    Item {
        anchors.fill: parent
        visible: root.mode === ""
        z: 5

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: pickerRow.top
            anchors.bottomMargin: 12
            text: qsTr("Choose 2D or 3D")
            color: AppPalette.textSecond
            font.pixelSize: 16
        }

        Row {
            id: pickerRow
            anchors.centerIn: parent
            spacing: 12

            CircleIconButton {
                width: 64
                height: 64
                glyph: "2D"
                glyphPixelSize: 20
                enabled: root.canPick2D
                toolTipText: root.canPick2D
                             ? qsTr("2D")
                             : qsTr("Echogram limit reached")
                fillColor: AppPalette.card
                fillHoverColor: AppPalette.cardHover
                fillPressedColor: AppPalette.bgDeep
                borderColor: AppPalette.border
                borderHoverColor: AppPalette.borderHover
                onClicked: if (root.store) root.store.setSecondaryWindowMode("2D")
            }

            CircleIconButton {
                width: 64
                height: 64
                glyph: "3D"
                glyphPixelSize: 20
                enabled: root.threeDSupported
                toolTipText: qsTr("3D in second window: coming soon")
                fillColor: AppPalette.card
                fillHoverColor: AppPalette.cardHover
                fillPressedColor: AppPalette.bgDeep
                borderColor: AppPalette.border
                borderHoverColor: AppPalette.borderHover
                onClicked: if (root.store) root.store.setSecondaryWindowMode("3D")
            }
        }
    }

    // ── Reset-mode button (matches main HotActionsPanel gear position/size) ──
    CircleIconButton {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 20    // HotActionsPanel.leftMargin (12) + panelPaddingX (8)
        anchors.topMargin: 18     // HotActionsPanel.topMargin (12) + panelPaddingY (6)
        width: 34                 // HotActionsPanel.controlHeight
        height: 34
        glyph: "↺"
        glyphPixelSize: 18
        toolTipText: qsTr("Change mode")
        visible: root.mode !== ""
        z: 20
        onClicked: if (root.store) root.store.setSecondaryWindowMode("")
    }

    // ── 2D content (dedicated qPlot2D, activation gated by setSecondaryWindowMode) ──
    Loader {
        anchors.fill: parent
        visible: root.mode === "2D"
        active: root.mode === "2D"
        z: 2
        sourceComponent: plot2DComponent
    }

    Component {
        id: plot2DComponent
        Plot2D {
            indx: 6
            inputState: inputStateObject
            externalInputRouting: false
            is3dVisible: false

            Component.onCompleted: {
                setIndx(6)
                if (typeof core !== "undefined" && core && typeof core.registerPlot2D === "function")
                    core.registerPlot2D(this)
            }
        }
    }

}
