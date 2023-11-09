import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1
import KoggerCommon 1.0
import SceneObject 1.0

Item {
    readonly property var controller : SurfaceControlMenuController

    id: root

    Component.onCompleted: {
        var surface = root.controller.surface;

        visibilityCheckBox.checkState        = surface.visible ? Qt.Checked : Qt.Unchecked
        contourVisibilityCheckBox.checkState = surface.contour.visible ? Qt.Checked : Qt.Unchecked
        gridVisibilityCheckBox.checkState    = surface.grid.visible ? Qt.Checked : Qt.Unchecked
        gridInterpolationCheckBox.checkState = surface.isQuad ? Qt.Checked : Qt.Unchecked
        contourColorPickRect.color           = surface.contour.color
        gridColorPickRect.color              = surface.grid.color
        contourColorDialog.setCurrentColor(surface.contour.color)
        gridColorDialog.setCurrentColor(surface.grid.color)
    }

    Layout.minimumWidth:  160
    Layout.minimumHeight: 240

    MenuBlockEx {
        anchors.fill: parent

        KParamGroup {
            id: paramGroup
            anchors.left:    parent.left
            anchors.right:   parent.right
            anchors.top:     parent.top
            anchors.margins: 24
            spacing:         24
            groupName:       qsTr("Surface controls")

            KCheck {
                id:               visibilityCheckBox
                text:             qsTr("Show")
                onCheckedChanged: root.controller.onSurfaceVisibilityCheckBoxCheckedChanged(checked)
            }

            RowLayout {
                anchors.left:  paramGroup.left
                anchors.right: paramGroup.right

                KCheck {
                    id:               contourVisibilityCheckBox
                    text:             qsTr("Show contour")
                    Layout.fillWidth: true
                    onCheckedChanged: root.controller.onSurfaceContourVisibilityCheckBoxCheckedChanged(checked)
                }

                Rectangle {
                    id:               contourColorPickRect
                    color:            contourColorDialog.color
                    width:            60
                    height:           contourVisibilityCheckBox.height
                    Layout.alignment: Qt.AlignRight

                    MouseArea {
                        anchors.fill: parent
                        onClicked:    contourColorDialog.open()
                    }
                }

                ColorDialog {
                    id:         contourColorDialog
                    onAccepted: {
                        contourColorPickRect.color = contourColorDialog.color
                        root.controller.onContourColorDialogAccepted(contourColorDialog.color)
                    }
                }
            }

            RowLayout {
                anchors.left:  paramGroup.left
                anchors.right: paramGroup.right

                KCheck {
                    id:               gridVisibilityCheckBox
                    text:             qsTr("Show grid")
                    Layout.fillWidth: true
                    onCheckedChanged: root.controller.onSurfaceGridVisibilityCheckBoxCheckedChanged(checked)
                }

                Rectangle {
                    id:               gridColorPickRect
                    color:            gridColorDialog.color
                    width:            60
                    height:           gridVisibilityCheckBox.height
                    Layout.alignment: Qt.AlignRight

                    MouseArea {
                        anchors.fill: parent
                        onClicked:    gridColorDialog.open()
                    }
                }

                ColorDialog {
                    id:         gridColorDialog
                    onAccepted: {
                        gridColorPickRect.color = gridColorDialog.color
                        root.controller.onGridColorDialogAccepted(gridColorDialog.color)
                    }
                }
            }

            KCheck {
                id:               gridInterpolationCheckBox
                text:             qsTr("Enable grid interpolation")
                Layout.fillWidth: true
                onCheckedChanged: root.controller.onGridInterpolationCheckBoxCheckedChanged(checked)
            }

            KParamSetup {
                id: gridCellSizeLayout
                paramName: qsTr("Grid cell size: ")
                visible: gridInterpolationCheckBox.checked

                KSpinBox {
                    id: gridCellSizeSpinBox
                    from: 1
                    to: 100
                }
            }

            KButton {
                id:               updateSurfaceButton
                Layout.fillWidth: true
                text:             qsTr("Update surface")
                onClicked:        root.controller.onUpdateSurfaceButtonClicked(gridInterpolationCheckBox.checked,
                                                                             gridCellSizeSpinBox.value)
            }
        }
    }
}
