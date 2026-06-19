import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtCore
import kqml_types 1.0
import controls
import menus

Item {
    id: root

    property var geo: null
    property var view: null
    property var store: null
    property bool geometryOpen: false
    property real buttonSize: Math.round(40 * (theme ? theme.resCoeff : 1.0))
    property bool toolbarHovered: buttonColumnHoverHandler.hovered
    property bool toolbarPressed: rulerToolButton.pressed || navArrowButton.pressed
    property bool menuOpened: root.geometryOpen

    property bool layersOpen: false
    function toggleLayers() { layersOpen = false }
    function toggleGeometry() { geometryOpen = !geometryOpen }

    width: buttonColumn.width + 8

    HoverHandler {
        onHoveredChanged: {
            if (hovered && root.view && root.view.resetScenePointerState) {
                root.view.resetScenePointerState()
            }
        }
    }

    function _zoom(steps) {
        if (!root.view || typeof root.view.mouseWheelTrigger !== "function")
            return
        var cx = root.view.width  / 2
        var cy = root.view.height / 2
        root.view.mouseWheelTrigger(Qt.NoButton, cx, cy, Qt.point(0, steps * 120), 0)
    }

    ColumnLayout {
        id: buttonColumn
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.rightMargin: 12 + AppPalette.splitHitSizePx / 2
        spacing: Math.round(6 * AppPalette.scale)
        z: 3

        opacity: buttonColumnFade.value
        Behavior on opacity { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }
        IdleFade { id: buttonColumnFade; hovered: buttonColumnHoverHandler.hovered }

        HoverHandler {
            id: buttonColumnHoverHandler
        }

        KCircleIconButton {
            id: zoomInButton
            width: root.buttonSize
            height: root.buttonSize
            Layout.preferredWidth: root.buttonSize
            Layout.preferredHeight: root.buttonSize
            iconSource: "qrc:/icons/ui/zoom-in.svg"
            iconTintColor: AppPalette.text
            fillColor: AppPalette.card
            fillHoverColor: AppPalette.cardHover
            borderColor: AppPalette.border
            toolTipText: qsTr("Zoom in")
            onClicked: root._zoom(+4)
        }

        KCircleIconButton {
            id: zoomOutButton
            width: root.buttonSize
            height: root.buttonSize
            Layout.preferredWidth: root.buttonSize
            Layout.preferredHeight: root.buttonSize
            iconSource: "qrc:/icons/ui/zoom-out.svg"
            iconTintColor: AppPalette.text
            fillColor: AppPalette.card
            fillHoverColor: AppPalette.cardHover
            borderColor: AppPalette.border
            toolTipText: qsTr("Zoom out")
            onClicked: root._zoom(-4)
        }

        Item { Layout.preferredHeight: Tokens.spaceLg; Layout.preferredWidth: 1 }

        KCircleIconButton {
            id: navArrowButton
            objectName: "followBoatButton"
            width: root.buttonSize
            height: root.buttonSize
            Layout.preferredWidth: root.buttonSize
            Layout.preferredHeight: root.buttonSize
            iconSource: "qrc:/icons/ui/location.svg"
            iconTintColor: AppPalette.text
            fillHoverColor: AppPalette.cardHover
            toolTipText: qsTr("Follow boat")

            readonly property bool checked: root.store ? root.store.trackLastDataEnabled : false
            fillColor: checked ? AppPalette.accentBgStrong : AppPalette.card
            borderColor: checked ? AppPalette.accentBorder : AppPalette.border
            borderWidth: checked ? 2 : 1

            onClicked: {
                if (!root.store) return
                root.store.trackLastDataEnabled = !root.store.trackLastDataEnabled
                Scene3dToolBarController.onTrackLastDataCheckButtonCheckedChanged(root.store.trackLastDataEnabled)
            }

            Component.onCompleted: {
                Scene3dToolBarController.onTrackLastDataCheckButtonCheckedChanged(checked)
            }
        }

        KCircleIconButton {
            id: rulerToolButton
            objectName: "rulerToolButton"
            width: root.buttonSize
            height: root.buttonSize
            Layout.preferredWidth: root.buttonSize
            Layout.preferredHeight: root.buttonSize
            iconSource: "qrc:/icons/ui/ruler_measure.svg"
            iconTintColor: AppPalette.text
            fillHoverColor: AppPalette.cardHover
            toolTipText: qsTr("Ruler")

            property bool checked: false
            fillColor: checked ? AppPalette.accentBgStrong : AppPalette.card
            borderColor: checked ? AppPalette.accentBorder : AppPalette.border
            borderWidth: checked ? 2 : 1

            onClicked: {
                checked = !checked
                Scene3dToolBarController.onRulerModeChanged(checked)
            }

            Component.onCompleted: {
                Scene3dToolBarController.onRulerModeChanged(checked)
                if (root.view && root.view.rulerEnabled !== checked) {
                    root.view.rulerEnabled = checked
                }
            }

            Settings {
                property alias rulerToolButton: rulerToolButton.checked
            }
        }

        Connections {
            target: root.view
            function onRulerEnabledChanged() {
                if (root.view && rulerToolButton.checked !== root.view.rulerEnabled) {
                    rulerToolButton.checked = root.view.rulerEnabled
                }
            }
        }

        Item { Layout.preferredHeight: Tokens.spaceLg; Layout.preferredWidth: 1 }

        KCircleIconButton {
            id: settingsGearButton
            objectName: "settingsGearButton"
            width: root.buttonSize
            height: root.buttonSize
            Layout.preferredWidth: root.buttonSize
            Layout.preferredHeight: root.buttonSize
            iconSource: "qrc:/icons/ui/settings.svg"
            iconTintColor: AppPalette.text
            fillColor: AppPalette.card
            fillHoverColor: AppPalette.cardHover
            borderColor: AppPalette.border
            toolTipText: qsTr("3D scene settings")
            visible: root.store !== null

            onClicked: {
                if (root.store && typeof root.store.openAppSettingsAtGroup === "function")
                    root.store.toggleAppSettingsAtGroup("app.scene3d")
            }
        }

        ColumnLayout {
            spacing: Math.round(6 * AppPalette.scale)
            visible: root.geometryOpen || (root.geo && root.geo.tool !== 0)

            KCircleIconButton {
                width: root.buttonSize
                height: root.buttonSize
                Layout.preferredWidth: root.buttonSize
                Layout.preferredHeight: root.buttonSize
                iconSource: "qrc:/icons/ui/point.svg"
                iconTintColor: AppPalette.text
                fillHoverColor: AppPalette.cardHover
                toolTipText: qsTr("Point")
                property bool checked: root.geo ? root.geo.tool === 1 : false
                fillColor: checked ? AppPalette.accentBgStrong : AppPalette.card
                borderColor: checked ? AppPalette.accentBorder : AppPalette.border
                onClicked: if (root.geo) root.geo.tool = (root.geo.tool === 1 ? 0 : 1)
            }

            KCircleIconButton {
                width: root.buttonSize
                height: root.buttonSize
                Layout.preferredWidth: root.buttonSize
                Layout.preferredHeight: root.buttonSize
                iconSource: "qrc:/icons/ui/line.svg"
                iconTintColor: AppPalette.text
                fillHoverColor: AppPalette.cardHover
                toolTipText: qsTr("Line")
                property bool checked: root.geo ? root.geo.tool === 2 : false
                fillColor: checked ? AppPalette.accentBgStrong : AppPalette.card
                borderColor: checked ? AppPalette.accentBorder : AppPalette.border
                onClicked: if (root.geo) root.geo.tool = (root.geo.tool === 2 ? 0 : 2)
            }

            KCircleIconButton {
                width: root.buttonSize
                height: root.buttonSize
                Layout.preferredWidth: root.buttonSize
                Layout.preferredHeight: root.buttonSize
                iconSource: "qrc:/icons/ui/polygon.svg"
                iconTintColor: AppPalette.text
                fillHoverColor: AppPalette.cardHover
                toolTipText: qsTr("Polygon")
                property bool checked: root.geo ? root.geo.tool === 3 : false
                fillColor: checked ? AppPalette.accentBgStrong : AppPalette.card
                borderColor: checked ? AppPalette.accentBorder : AppPalette.border
                onClicked: if (root.geo) root.geo.tool = (root.geo.tool === 3 ? 0 : 3)
            }
        }

        ColumnLayout {
            spacing: Math.round(6 * AppPalette.scale)
            visible: root.geo ? root.geo.drawing : false

            KCircleIconButton {
                width: root.buttonSize
                height: root.buttonSize
                Layout.preferredWidth: root.buttonSize
                Layout.preferredHeight: root.buttonSize
                iconSource: "qrc:/icons/ui/file-check.svg"
                iconTintColor: AppPalette.text
                fillHoverColor: AppPalette.cardHover
                toolTipText: qsTr("Finish drawing")
                enabled: root.geo ? root.geo.drawing : false
                onClicked: if (root.geo) root.geo.finishDrawing()
            }

            KCircleIconButton {
                width: root.buttonSize
                height: root.buttonSize
                Layout.preferredWidth: root.buttonSize
                Layout.preferredHeight: root.buttonSize
                iconSource: "qrc:/icons/ui/repeat.svg"
                iconTintColor: AppPalette.text
                fillHoverColor: AppPalette.cardHover
                toolTipText: qsTr("Undo")
                enabled: root.geo ? root.geo.drawing : false
                onClicked: if (root.geo) root.geo.undoLastVertex()
            }

            KCircleIconButton {
                width: root.buttonSize
                height: root.buttonSize
                Layout.preferredWidth: root.buttonSize
                Layout.preferredHeight: root.buttonSize
                iconSource: "qrc:/icons/ui/x.svg"
                iconTintColor: AppPalette.text
                fillHoverColor: AppPalette.cardHover
                toolTipText: qsTr("Cancel drawing")
                enabled: root.geo ? root.geo.drawing : false
                onClicked: if (root.geo) root.geo.cancelDrawing()
            }
        }
    }

    Scene3DGeometryPanel {
        id: geometryPanel
        expanded: root.geometryOpen
        geo: root.geo
        view: root.view
        anchors.right: buttonColumn.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        z: 2
    }
}
