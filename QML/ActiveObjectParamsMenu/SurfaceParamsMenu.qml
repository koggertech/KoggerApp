import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1
import KoggerCommon 1.0

Item {
    id: root

    property var activeObject: null

    function setActiveObject(object){
        activeObject = object

        if (!object)
            return

        visibilityCheckBox.checked = activeObject.visible

        initSourceBottomTrackParams()

        var contour = activeObject.contour
        var grid    = activeObject.grid

        contourVisibilityCheckBox.checked = contour.visible
        contourColorPickRect.color        = contour.color
        gridVisibilityCheckBox.checked    = grid.visible
        gridColorPickRect.color           = grid.color
    }

    function initSourceBottomTrackParams(){
        var bottomTrackIndex = SceneObjectListModel.objectIndex(activeObject.bottomTrackId);

        messageItem.visible = bottomTrackIndex === -1

        if(bottomTrackIndex !== -1){
            surfaceSourceCombo.currentIndex = bottomTrackIndex
        }
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
            groupName:       activeObject.name

            KCheck {
                id:               visibilityCheckBox
                text:             "Show"
                onCheckedChanged: SurfaceParamsController.changeSurfaceVisibility(checked)
            }

            RowLayout {
                anchors.left:  paramGroup.left
                anchors.right: paramGroup.right

                KCheck {
                    id:               contourVisibilityCheckBox
                    text:             "Show contour"
                    Layout.fillWidth: true
                    onCheckedChanged: SurfaceParamsController.changeSurfaceContourVisibility(checked)
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

                        SurfaceParamsController.changeSurfaceContourColor(contourColorDialog.color)
                    }
                }
            }

            RowLayout {
                anchors.left:  paramGroup.left
                anchors.right: paramGroup.right

                KCheck {
                    id:               gridVisibilityCheckBox
                    text:             "Show grid"
                    Layout.fillWidth: true
                    onCheckedChanged: SurfaceParamsController.changeSurfaceGridVisibility(checked)
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

                        SurfaceParamsController.changeSurfaceGridColor(gridColorDialog.color)
                    }
                }
            }

            KParamSetup {
                paramName: "Source: "

                KCombo {
                    id:               surfaceSourceCombo
                    Layout.fillWidth: true
                    model:            SceneObjectListModel.names("Bottom track")

                    Connections {
                        target:         SceneObjectListModel
                        onCountChanged: {
                            surfaceSourceCombo.model = SceneObjectListModel.names("Bottom track")

                            //initSourceBottomTrackParams()
                        }
                    }
                }
            }

            KCheck {
                id:               gridInterpolationCheckBox
                text:             "Enable grid interpolation"
                Layout.fillWidth: true
            }

            KParamSetup {
                id: gridCellSizeLayout
                paramName: "Grid cell size: "
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
                text:             "Update surface"
                onClicked:        SurfaceParamsController.updateSurface(surfaceSourceCombo.currentIndex,
                                                                        gridInterpolationCheckBox.checked,
                                                                        gridCellSizeSpinBox.value)
            }

            Text {
                id:               messageItem
                Layout.fillWidth: true
                color:            theme.textColor
                text:             "Surface source data is not available. " +
                                  "Please, choose bottom track\nsource" +
                                  "from list and press 'Update' button"
            }
        }
    }

    Connections {
        target: activeObject
        onBottomTrackIdChanged: {
            initSourceBottomTrackParams()
        }
    }

}
