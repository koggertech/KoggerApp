import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12


ColumnLayout {
    Layout.alignment: Qt.AlignHCenter

    // surface extra settings
    MenuFrame {
        id: surfaceSettings
        visible: surfaceCheckButton.checked && (surfaceCheckButton.hovered || isHovered)
        z: surfaceSettings.visible
        Layout.alignment: Qt.AlignHCenter

        onIsHoveredChanged: {
            console.debug("surface menu hovered " + isHovered.toString())
        }

        ColumnLayout {
            width: 300
            ParamSetup {
                paramName: "Edge limit, m:"

                SpinBoxCustom {
                    id: triangleEdgeLengthLimitSpinBox
                    implicitWidth: 110
                    from: 5
                    to: 200
                    stepSize: 5
                    value: 50
                }
            }

            ParamSetup {
                paramName: "Decimation by:"

                CheckButton {
                    id: decimationCountCheck
                    text: "Count"
                    checked: true
                    ButtonGroup.group: decimationGroup
                }

                CheckButton {
                    id: decimationDistanceCheck
                    text: "Distance"
                    ButtonGroup.group: decimationGroup
                }

                // CheckButton {
                //     icon.source: "./icons/x.svg"
                //     ButtonGroup.group: decimationGroup
                // }

                ButtonGroup{
                    id: decimationGroup
                }
            }

            ParamSetup {
                visible: decimationCountCheck.checked
                paramName: "Point count:"

                SpinBoxCustom {
                    id: decimationCountSpinBox
                    from: 100
                    to: 10000
                    stepSize: 100
                    value: 1000
                }
            }

            ParamSetup {
                //id: decimationDistance
                visible: decimationDistanceCheck.checked
                paramName: "Decimation, m:"

                SpinBoxCustom {
                    id: decimationDistanceSpinBox
                    implicitWidth: 150
                    from: 1
                    to: 100
                    stepSize: 1
                    value: 5
                }
            }

            ParamSetup {
                paramName: "Type:"

                CheckButton {
                    id: triangleTypeCheck
                    text: "Triangle"
                    checked: true
                    ButtonGroup.group: surfaceTypeGroup
                }

                CheckButton {
                    id: gridTypeCheck
                    text: "Grid"
                    ButtonGroup.group: surfaceTypeGroup
                }

                ButtonGroup{
                    id: surfaceTypeGroup
                }
            }

            ParamSetup {
                visible: gridTypeCheck.checked
                paramName: "Grid step, m:"

                SpinBoxCustom {
                    id: gridCellSizeSpinBox
                    implicitWidth: 150
                    from: 1
                    to: 20
                    stepSize: 1
                    value: 5
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter

                CheckButton {
                    id: contourVisibilityCheckButton
                    text: qsTr("Show contour")
                    //checked: true
                    Layout.fillWidth: true
                    onToggled: SurfaceControlMenuController.onSurfaceContourVisibilityCheckBoxCheckedChanged(checked)
                }
                CheckButton {
                    id: gridVisibilityCheckButton
                    text: qsTr("Show grid")
                    //checked: true
                    Layout.fillWidth: true
                    onToggled: SurfaceControlMenuController.onSurfaceGridVisibilityCheckBoxCheckedChanged(checked)
                }
            }

            CButton {
                text: "Update"
                Layout.fillWidth: true
                icon.source: "./icons/refresh.svg"
                onClicked: SurfaceControlMenuController.onUpdateSurfaceButtonClicked(
                               triangleEdgeLengthLimitSpinBox.value,
                               !gridTypeCheck.checked ? -1: gridCellSizeSpinBox.value,
                               !decimationCountCheck.checked ? -1 : decimationCountSpinBox.value,
                               !decimationDistanceCheck.checked ? -1 : decimationDistanceSpinBox.value)
            }
        }
    }

    RowLayout {
        spacing: 2
        Layout.alignment: Qt.AlignHCenter

        MenuButton {
            id: setCameraIsometricView
            width: theme.controlHeight
            height: theme.controlHeight
            icon.source: "./fit-in-view.svg"

            icon.color: theme.textColor

            onClicked: Scene3dToolBarController.onSetCameraMapViewButtonClicked()
        }

        MenuButton {
            id: fitAllinViewButton
            width: theme.controlHeight
            height: theme.controlHeight
            icon.source: "./icons/zoom-cancel.svg"


            icon.color: theme.textColor

            onClicked: Scene3dToolBarController.onFitAllInViewButtonClicked()
        }

        MenuButton {
            id: selectionToolButton
            width: theme.controlHeight
            height: theme.controlHeight
            checkable: true
            active: checked
            icon.source: "./icons/click.svg"
            icon.color: theme.textColor
            ButtonGroup.group: buttonGroup

            onCheckedChanged: Scene3dToolBarController.onBottomTrackVertexEditingModeButtonChecked(checked)
        }

        MenuButton {
            id: bottomTrackVertexComboSelectionToolButton
            width: theme.controlHeight
            height: theme.controlHeight
            checkable: true
            active: checked
            icon.source: "./combo-selection.svg"
            icon.color: theme.textColor
            ButtonGroup.group: buttonGroup

            onCheckedChanged: Scene3dToolBarController.onBottomTrackVertexComboSelectionModeButtonChecked(checked)
        }

        CheckButton {
            id: boatTrackCheckButton
            implicitHeight: theme.controlHeight
            implicitWidth: theme.controlHeight
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true

            icon.source: "./icons/route.svg"
            // icon.width: width
            // icon.height: height

            onCheckedChanged: {
                BoatTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
            }

            Component.onCompleted: {
                BoatTrackTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
            }
        }

        CheckButton {
            id: bottomTrackCheckButton
            implicitHeight: theme.controlHeight
            implicitWidth: theme.controlHeight
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true

            icon.source: "./icons/overline.svg"

            onCheckedChanged: {
                BottomTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
            }

            Component.onCompleted: {
                BottomTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
            }
        }

        CheckButton {
            id: surfaceCheckButton
            implicitHeight: theme.controlHeight
            implicitWidth: theme.controlHeight
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true
            // hoverEnabled: true

            onCheckedChanged: {                
                SurfaceControlMenuController.onSurfaceVisibilityCheckBoxCheckedChanged(checked)
                SurfaceControlMenuController.onSurfaceContourVisibilityCheckBoxCheckedChanged(checked)
                SurfaceControlMenuController.onSurfaceGridVisibilityCheckBoxCheckedChanged(checked)
                contourVisibilityCheckButton.checked = checked
                gridVisibilityCheckButton.checked = checked
            }

            Component.onCompleted: {
                SurfaceControlMenuController.onSurfaceVisibilityCheckBoxCheckedChanged(checked)
                SurfaceControlMenuController.onSurfaceContourVisibilityCheckBoxCheckedChanged(checked)
                SurfaceControlMenuController.onSurfaceGridVisibilityCheckBoxCheckedChanged(checked)
                contourVisibilityCheckButton.checked = checked
                gridVisibilityCheckButton.checked = checked
            }

            icon.source: "./icons/stack-backward.svg"
            // onCheckedChanged: Scene3dToolBarController.onBottomTrackVertexComboSelectionModeButtonChecked(checked)
        }


        ButtonGroup{
            property bool buttonChangeFlag : false
            id: buttonGroup
            onCheckedButtonChanged: buttonChangeFlag = true
            onClicked: {
                if(!buttonChangeFlag)
                    checkedButton = null

                buttonChangeFlag = false;
            }
        }
    }
}
