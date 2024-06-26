import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12


ColumnLayout {
    Layout.alignment: Qt.AlignHCenter

    // surface extra settings
    MenuFrame {
        id: surfaceSettings
        visible: surfaceCheckButton.checked && (surfaceCheckButton.hovered || isHovered || surfaceCheckButton.longPressTriggered)
        z: surfaceSettings.visible
        Layout.alignment: Qt.AlignHCenter

        onIsHoveredChanged: {
            if (!isHovered || !surfaceCheckButton.hovered)
                surfaceCheckButton.longPressTriggered = false
            //console.debug("surface menu hovered " + isHovered.toString())
        }

        onVisibleChanged: {
            if (visible)
                focus = true;
        }

        onFocusChanged: {
            console.info("surfaceSettings onFocusChanged: " + focus)
            if (!focus) {
                surfaceCheckButton.longPressTriggered = false
            }
        }

        ColumnLayout {
            //width: 300
            ParamSetup {
                paramName: "Edge limit, m:"

                SpinBoxCustom {
                    id: triangleEdgeLengthLimitSpinBox
                    //implicitWidth: 110
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
                onClicked: {
                    SurfaceControlMenuController.onUpdateSurfaceButtonClicked(
                        triangleEdgeLengthLimitSpinBox.value,
                        !gridTypeCheck.checked ? -1: gridCellSizeSpinBox.value,
                        !decimationCountCheck.checked ? -1 : decimationCountSpinBox.value,
                        !decimationDistanceCheck.checked ? -1 : decimationDistanceSpinBox.value)
                    //BottomTrackControlMenuController.onSurfaceUpdated()
                }
            }
        }
    }

    RowLayout {
        spacing: 2
        Layout.alignment: Qt.AlignHCenter


        CheckButton {
            id: setCameraIsometricView
            backColor: theme.controlBackColor
            iconSource: "./fit-in-view.svg"
            checkable: false
            checked: false
            implicitWidth: theme.controlHeight

            onClicked: Scene3dToolBarController.onSetCameraMapViewButtonClicked()
        }

        CheckButton {
            id: fitAllinViewButton
            iconSource: "./icons/zoom-cancel.svg"
            backColor: theme.controlBackColor
            checkable: false
            checked: false
            implicitWidth: theme.controlHeight

            onClicked: Scene3dToolBarController.onFitAllInViewButtonClicked()
        }

        CheckButton {
            id: selectionToolButton
            objectName: "selectionToolButton"
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true
            iconSource: "./icons/click.svg"
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                Scene3dToolBarController.onBottomTrackVertexEditingModeButtonChecked(checked)
            }
        }

        CheckButton {
            id: boatTrackCheckButton
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true
            iconSource: "./icons/route.svg"
            implicitWidth: theme.controlHeight


            onCheckedChanged: {
                BoatTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
            }

            Component.onCompleted: {
                BoatTrackTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
            }
        }

        CheckButton {
            id: bottomTrackCheckButton
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true
            iconSource: "./icons/overline.svg"
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                BottomTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
            }

            Component.onCompleted: {
                BottomTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
            }
        }

        CheckButton {
            id: surfaceCheckButton
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            checked: true
            iconSource: "./icons/stack-backward.svg"
            implicitWidth: theme.controlHeight

            onCheckedChanged: {
                SurfaceControlMenuController.onSurfaceVisibilityCheckBoxCheckedChanged(checked)
                SurfaceControlMenuController.onSurfaceContourVisibilityCheckBoxCheckedChanged(checked)
                SurfaceControlMenuController.onSurfaceGridVisibilityCheckBoxCheckedChanged(checked)
                contourVisibilityCheckButton.checked = checked
                gridVisibilityCheckButton.checked = checked
                BottomTrackControlMenuController.onSurfaceStateChanged(checked)
            }

            Component.onCompleted: {
                SurfaceControlMenuController.onSurfaceVisibilityCheckBoxCheckedChanged(checked)
                SurfaceControlMenuController.onSurfaceContourVisibilityCheckBoxCheckedChanged(checked)
                SurfaceControlMenuController.onSurfaceGridVisibilityCheckBoxCheckedChanged(checked)
                contourVisibilityCheckButton.checked = checked
                gridVisibilityCheckButton.checked = checked
                BottomTrackControlMenuController.onSurfaceStateChanged(checked)
            }

            onFocusChanged: {
                console.info("surfaceCheckButton onFocusChanged: " + focus)
            }

            property bool longPressTriggered: false

            MouseArea {
                id: touchArea
                anchors.fill: parent
                onPressed: {
                    longPressTimer.start()
                    surfaceCheckButton.longPressTriggered = false
                }

                onReleased: {
                    if (!surfaceCheckButton.longPressTriggered) {
                        surfaceCheckButton.checked = !surfaceCheckButton.checked
                    }
                    longPressTimer.stop()
                }

                onCanceled: {
                    longPressTimer.stop()
                }
            }

            Timer {
                id: longPressTimer
                interval: 700 // ms
                repeat: false
                onTriggered: {
                    surfaceCheckButton.longPressTriggered = true;
                }
            }
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
