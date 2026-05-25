import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtCore
import kqml_types 1.0
import "../controls"
import "../menus"

Item {
    id: root

    property var geo: null
    property var view: null
    // Kept for backwards compatibility with Pane3DWindow.closeTransientUi().
    // Map-tile-provider menu lives in AppSettings now; this property is always
    // false and the toggleLayers() function is a no-op.
    property bool layersOpen: false
    property bool geometryOpen: false
    property real buttonSize: theme.controlHeight * 1.3
    property bool toolbarHovered: buttonColumnHoverHandler.hovered
    property bool toolbarPressed: rulerToolButton.down || zoomInButton.down || zoomOutButton.down
    property bool menuOpened: root.geometryOpen

    width: buttonColumn.width + geometryPanel.width + 8
    opacity: (toolbarHovered || toolbarPressed || menuOpened) ? 1.0 : 0.5
    Behavior on opacity { NumberAnimation { duration: 120 } }

    // One mouse-wheel step at the centre of the 3D viewport. The C++ side
    // divides angleDelta.y by 120 to get logical steps and clamps to ±8.
    function _zoom(steps) {
        if (!root.view || typeof root.view.mouseWheelTrigger !== "function")
            return
        var cx = root.view.width  / 2
        var cy = root.view.height / 2
        root.view.mouseWheelTrigger(Qt.NoButton, cx, cy, Qt.point(0, steps * 120), 0)
    }

    HoverHandler {
        onHoveredChanged: {
            if (hovered && root.view && root.view.resetScenePointerState) {
                root.view.resetScenePointerState()
            }
        }
    }

    function toggleLayers() { /* moved to AppSettings; kept for ESC compatibility */ }

    function toggleGeometry() {
        geometryOpen = !geometryOpen
    }

    ColumnLayout {
        id: buttonColumn
        anchors.right: parent.right
        // Vertically centred against the pane — Ruler sits in the middle,
        // zoom-in / zoom-out stack right above it.
        anchors.verticalCenter: parent.verticalCenter
        // Stay clear of split-drag hit zone on the right edge.
        anchors.rightMargin: 12 + AppPalette.splitHitSizePx / 2
        spacing: 6
        z: 3

        HoverHandler {
            id: buttonColumnHoverHandler
        }

        CheckButton {
            id: zoomInButton
            checkable: false
            iconSource: "qrc:/icons/ui/zoom-in.svg"
            implicitWidth: buttonSize
            implicitHeight: buttonSize
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor

            CMouseOpacityArea {
                toolTipText: qsTr("Zoom in")
                popupPosition: "bottomLeft"
            }

            onClicked: root._zoom(+4)
        }

        CheckButton {
            id: zoomOutButton
            checkable: false
            iconSource: "qrc:/icons/ui/zoom-out.svg"
            implicitWidth: buttonSize
            implicitHeight: buttonSize
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor

            CMouseOpacityArea {
                toolTipText: qsTr("Zoom out")
                popupPosition: "bottomLeft"
            }

            onClicked: root._zoom(-4)
        }

        CheckButton {
            id: rulerToolButton
            checkable: true
            checked: false
            iconSource: "qrc:/icons/ui/ruler_measure.svg"
            implicitWidth: buttonSize
            implicitHeight: buttonSize
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor

            CMouseOpacityArea {
                toolTipText: qsTr("Ruler")
                popupPosition: "bottomLeft"
            }

            onToggled: {
                Scene3dToolBarController.onRulerModeChanged(checked)
            }

            Component.onCompleted: {
                if (root.view) {
                    checked = root.view.rulerEnabled
                }
                Scene3dToolBarController.onRulerModeChanged(checked)
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

        CheckButton {
            checkable: false
            iconSource: "qrc:/icons/ui/map_pin_cog.svg"
            implicitWidth: buttonSize
            implicitHeight: buttonSize
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            visible: false

            CMouseOpacityArea {
                toolTipText: qsTr("GeoJSON")
                popupPosition: "bottomLeft"
            }

            onClicked: root.toggleGeometry()
        }

        ColumnLayout {
            spacing: 6
            visible: root.geometryOpen || (geo && geo.tool !== 0)

            CheckButton {
                checkable: true
                checked: geo ? geo.tool === 1 : false
                iconSource: "qrc:/icons/ui/point.svg"
                implicitWidth: buttonSize
                implicitHeight: buttonSize
                backColor: theme.controlBackColor
                borderColor: theme.controlBackColor
                checkedBorderColor: theme.controlBorderColor

                CMouseOpacityArea {
                    toolTipText: qsTr("Point")
                    popupPosition: "bottomLeft"
                }

                onClicked: {
                    if (geo) geo.tool = (geo.tool === 1 ? 0 : 1)
                }
            }

            CheckButton {
                checkable: true
                checked: geo ? geo.tool === 2 : false
                iconSource: "qrc:/icons/ui/line.svg"
                implicitWidth: buttonSize
                implicitHeight: buttonSize
                backColor: theme.controlBackColor
                borderColor: theme.controlBackColor
                checkedBorderColor: theme.controlBorderColor

                CMouseOpacityArea {
                    toolTipText: qsTr("Line")
                    popupPosition: "bottomLeft"
                }

                onClicked: {
                    if (geo) geo.tool = (geo.tool === 2 ? 0 : 2)
                }
            }

            CheckButton {
                checkable: true
                checked: geo ? geo.tool === 3 : false
                iconSource: "qrc:/icons/ui/polygon.svg"
                implicitWidth: buttonSize
                implicitHeight: buttonSize
                backColor: theme.controlBackColor
                borderColor: theme.controlBackColor
                checkedBorderColor: theme.controlBorderColor

                CMouseOpacityArea {
                    toolTipText: qsTr("Polygon")
                    popupPosition: "bottomLeft"
                }

                onClicked: {
                    if (geo) geo.tool = (geo.tool === 3 ? 0 : 3)
                }
            }
        }

        ColumnLayout {
            spacing: 6
            visible: geo ? geo.drawing : false

            CheckButton {
                checkable: false
                iconSource: "qrc:/icons/ui/file-check.svg"
                implicitWidth: buttonSize
                implicitHeight: buttonSize
                backColor: theme.controlBackColor
                borderColor: theme.controlBackColor
                checkedBorderColor: theme.controlBorderColor
                enabled: geo ? geo.drawing : false

                CMouseOpacityArea {
                    toolTipText: qsTr("Finish drawing")
                    popupPosition: "bottomLeft"
                }

                onClicked: if (geo) geo.finishDrawing()
            }

            CheckButton {
                checkable: false
                iconSource: "qrc:/icons/ui/repeat.svg"
                implicitWidth: buttonSize
                implicitHeight: buttonSize
                backColor: theme.controlBackColor
                borderColor: theme.controlBackColor
                checkedBorderColor: theme.controlBorderColor
                enabled: geo ? geo.drawing : false

                CMouseOpacityArea {
                    toolTipText: qsTr("Undo")
                    popupPosition: "bottomLeft"
                }

                onClicked: if (geo) geo.undoLastVertex()
            }

            CheckButton {
                checkable: false
                iconSource: "qrc:/icons/ui/x.svg"
                implicitWidth: buttonSize
                implicitHeight: buttonSize
                backColor: theme.controlBackColor
                borderColor: theme.controlBackColor
                checkedBorderColor: theme.controlBorderColor
                enabled: geo ? geo.drawing : false

                CMouseOpacityArea {
                    toolTipText: qsTr("Cancel drawing")
                    popupPosition: "bottomLeft"
                }

                onClicked: if (geo) geo.cancelDrawing()
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
