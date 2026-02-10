import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtCore

Item {
    id: root

    property var geo: null
    property var view: null
    property bool layersOpen: false
    property bool geometryOpen: false
    property real buttonSize: theme.controlHeight * 1.3

    width: buttonColumn.width + Math.max(layerPanel.width, geometryPanel.width) + 8

    function toggleLayers() {
        layersOpen = !layersOpen
        if (layersOpen) geometryOpen = false
    }

    function toggleGeometry() {
        geometryOpen = !geometryOpen
        if (geometryOpen) layersOpen = false
    }

    ColumnLayout {
        id: buttonColumn
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 12
        anchors.rightMargin: 12
        spacing: 6
        z: 3

        CheckButton {
            checkable: false
            iconSource: "qrc:/icons/ui/map.svg"
            implicitWidth: buttonSize
            implicitHeight: buttonSize
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            onClicked: root.toggleLayers()
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
                onClicked: if (geo) geo.cancelDrawing()
            }
        }
    }

    Scene3DLayerPanel {
        id: layerPanel
        showToggleButton: false
        expanded: root.layersOpen
        anchors.right: buttonColumn.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        z: 2

        onRequestClose: {
            root.layersOpen = false
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
