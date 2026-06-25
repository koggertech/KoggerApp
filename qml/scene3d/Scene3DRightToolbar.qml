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

    function dismissRuler() {
        if (!rulerControl.menuOpen) return
        if (rulerControl.hasGeometry) { if (root.view) root.view.rulerFinishDrawing() }
        else                         { if (root.view) root.view.clearRuler() }
        rulerControl._setOpen(false)
    }

    Connections {
        target: core
        function onActiveTransientUiChanged(who) { if (who !== root) root.dismissRuler() }
    }

    width: buttonColumn.width + 8

    HoverHandler {
        onHoveredChanged: {
            if (hovered && root.view && root.view.resetScenePointerState) {
                root.view.resetScenePointerState()
            }
        }
    }

    function _zoom(steps) {
        if (!root.view)
            return
        if (root.view.zoomButtonAnimated) {
            root.view.zoomButtonAnimated(steps)
            return
        }
        if (typeof root.view.mouseWheelTrigger !== "function")
            return
        var cx = root.view.width  / 2
        var cy = root.view.height / 2
        root.view.mouseWheelTrigger(Qt.NoButton, cx, cy, Qt.point(0, steps * 120), 0)
    }

    function cancelRuler() {
        if (root.view && root.view.rulerEnabled) {
            root.view.clearRuler()
            Scene3dToolBarController.onRulerModeChanged(false)
        }
    }

    MouseArea {
        anchors.fill: buttonColumn
        acceptedButtons: Qt.AllButtons
        hoverEnabled: true
        z: 2
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
        IdleFade { id: buttonColumnFade; hovered: buttonColumnHoverHandler.hovered || rulerControl.menuOpen || (root.view && root.view.followReturnPending) }

        HoverHandler {
            id: buttonColumnHoverHandler
        }

        KCircleIconButton {
            id: resetZoomButton
            readonly property bool shown: root.view && Math.abs(root.view.verticalScale - 1.0) > 0.001
            width: root.buttonSize
            Layout.preferredWidth: root.buttonSize
            Layout.preferredHeight: shown ? root.buttonSize : 0
            visible: Layout.preferredHeight > 1
            opacity: shown ? 1.0 : 0.0
            clip: true
            Behavior on Layout.preferredHeight { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
            Behavior on opacity { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
            iconSource: "qrc:/icons/ui/zoom_cancel.svg"
            iconTintColor: AppPalette.text
            fillColor: AppPalette.card
            fillHoverColor: AppPalette.cardHover
            borderColor: AppPalette.border
            toolTipText: qsTr("Reset zoom")
            onClicked: if (root.view) root.view.resetVerticalScale()
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
            onClicked: { root.cancelRuler(); root._zoom(+4) }
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
            onClicked: { root.cancelRuler(); root._zoom(-4) }
        }

        Item { Layout.preferredHeight: Tokens.spaceLg; Layout.preferredWidth: 1 }

        Item {
            id: navArrowControl
            width: root.buttonSize
            height: root.buttonSize
            Layout.preferredWidth: root.buttonSize
            Layout.preferredHeight: root.buttonSize

            readonly property real _s: theme ? theme.resCoeff : 1.0
            readonly property int _pad: Math.round(5 * _s)
            readonly property int _gap: Math.round(6 * AppPalette.scale)
            readonly property bool pending: navArrowButton.checked && root.view && root.view.followReturnPending
            readonly property real _openW: _pad * 2 + root.buttonSize * 2 + _gap

            Rectangle {
                id: followBacking
                z: -1
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: -navArrowControl._pad
                height: root.buttonSize + navArrowControl._pad * 2
                width: navArrowControl.pending ? navArrowControl._openW : 0
                radius: height / 2
                color: AppPalette.bg
                border.width: 1
                border.color: AppPalette.border
                opacity: navArrowControl.pending ? 1 : 0
                visible: opacity > 0.01
                clip: true

                Behavior on width   { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
                Behavior on opacity { NumberAnimation { duration: 170; easing.type: Easing.OutCubic } }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.AllButtons
                    hoverEnabled: true
                }

                Rectangle {
                    id: frDigit
                    anchors.right: parent.right
                    anchors.rightMargin: navArrowControl._pad + root.buttonSize + navArrowControl._gap
                    anchors.verticalCenter: parent.verticalCenter
                    width: root.buttonSize
                    height: root.buttonSize
                    radius: height / 2
                    color: AppPalette.card
                    border.color: AppPalette.accentBorder
                    border.width: 2

                    Text {
                        anchors.centerIn: parent
                        text: root.view ? root.view.followReturnSeconds : 0
                        color: AppPalette.text
                        font: theme.textFont
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (root.view) root.view.returnToBoatNow()
                    }
                }
            }

            KCircleIconButton {
                id: navArrowButton
                objectName: "followBoatButton"
                anchors.fill: parent
                iconSource: "qrc:/icons/ui/location.svg"
                iconTintColor: AppPalette.text
                fillHoverColor: AppPalette.cardHover
                toolTipText: qsTr("Follow boat")

                readonly property bool checked: root.store ? root.store.trackLastDataEnabled : false
                fillColor: checked ? AppPalette.accentBgStrong : AppPalette.card
                borderColor: checked ? AppPalette.accentBorder : AppPalette.border
                borderWidth: checked ? 2 : 1

                onClicked: {
                    root.cancelRuler()
                    if (!root.store) return
                    root.store.trackLastDataEnabled = !root.store.trackLastDataEnabled
                    Scene3dToolBarController.onTrackLastDataCheckButtonCheckedChanged(root.store.trackLastDataEnabled)
                    if (root.store.trackLastDataEnabled && root.view && root.view.flyToLastPosition)
                        root.view.flyToLastPosition()
                }

                Component.onCompleted: {
                    Scene3dToolBarController.onTrackLastDataCheckButtonCheckedChanged(checked)
                }
            }
        }

        Item {
            id: rulerControl
            width: root.buttonSize
            height: root.buttonSize
            Layout.preferredWidth: root.buttonSize
            Layout.preferredHeight: root.buttonSize

            readonly property bool menuOpen: root.view ? root.view.rulerEnabled : false
            readonly property real _s: theme ? theme.resCoeff : 1.0
            readonly property int _pad: Math.round(5 * _s)
            readonly property int _gap: Math.round(6 * AppPalette.scale)
            readonly property real _openW: _pad * 2 + root.buttonSize * 2 + _gap
            readonly property bool hasGeometry: root.view ? root.view.rulerHasGeometry : false

            function _setOpen(open) {
                Scene3dToolBarController.onRulerModeChanged(open)
            }

            onMenuOpenChanged: if (menuOpen && typeof core !== "undefined" && core) core.setActiveTransientUi(root)

            Rectangle {
                id: rulerBacking
                z: -1
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: -rulerControl._pad
                height: root.buttonSize + rulerControl._pad * 2
                width: rulerControl.menuOpen ? rulerControl._openW : 0
                radius: height / 2
                color: AppPalette.bg
                border.width: 1
                border.color: AppPalette.border
                opacity: rulerControl.menuOpen ? 1 : 0
                visible: opacity > 0.01
                clip: true

                Behavior on width   { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
                Behavior on opacity { NumberAnimation { duration: 170; easing.type: Easing.OutCubic } }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.AllButtons
                    hoverEnabled: true
                }

                Row {
                    anchors.right: parent.right
                    anchors.rightMargin: rulerControl._pad
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: rulerControl._gap

                    KCircleIconButton {
                        width: root.buttonSize
                        height: root.buttonSize
                        iconSource: "qrc:/icons/ui/check.svg"
                        iconTintColor: AppPalette.text
                        fillColor: AppPalette.card
                        fillHoverColor: AppPalette.cardHover
                        borderColor: AppPalette.border
                        toolTipText: qsTr("Save ruler")
                        enabled: rulerControl.hasGeometry
                        onClicked: {
                            if (root.view) root.view.rulerFinishDrawing()
                            rulerControl._setOpen(false)
                        }
                    }

                    KCircleIconButton {
                        width: root.buttonSize
                        height: root.buttonSize
                        iconSource: "qrc:/icons/ui/x.svg"
                        iconTintColor: AppPalette.text
                        fillColor: AppPalette.card
                        fillHoverColor: AppPalette.cardHover
                        borderColor: AppPalette.border
                        toolTipText: qsTr("Delete ruler")
                        onClicked: {
                            if (root.view) root.view.clearRuler()
                            rulerControl._setOpen(false)
                        }
                    }
                }
            }

            KCircleIconButton {
                id: rulerToolButton
                objectName: "rulerToolButton"
                anchors.fill: parent
                visible: !rulerControl.menuOpen
                iconSource: "qrc:/icons/ui/ruler_measure.svg"
                iconTintColor: AppPalette.text
                fillHoverColor: AppPalette.cardHover
                fillColor:   rulerControl.hasGeometry ? AppPalette.accentBgStrong : AppPalette.card
                borderColor: rulerControl.hasGeometry ? AppPalette.accentBorder : AppPalette.border
                borderWidth: rulerControl.hasGeometry ? 2 : 1
                toolTipText: qsTr("Ruler")
                onClicked: rulerControl._setOpen(true)
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
                root.cancelRuler()
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
                onClicked: { root.cancelRuler(); if (root.geo) root.geo.tool = (root.geo.tool === 1 ? 0 : 1) }
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
                onClicked: { root.cancelRuler(); if (root.geo) root.geo.tool = (root.geo.tool === 2 ? 0 : 2) }
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
                onClicked: { root.cancelRuler(); if (root.geo) root.geo.tool = (root.geo.tool === 3 ? 0 : 3) }
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
