import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

Item {

    Layout.fillWidth: true
    Layout.preferredHeight: settings3DLayout.height

    property bool displayScene: displaySceneCheckBox.checked

    MenuBlock {
    }

    ColumnLayout {
        id: settings3DLayout
        Layout.margins: 24
        width: parent.width

        ParamGroup {
            groupName: "Common"
            Layout.margins: 10

            CCheck {
                id: displaySceneCheckBox
                Layout.fillWidth: true
                checked: Scene3DModel.sceneVisibility()
                text: "Display 3D scene"
                onCheckedChanged: Settings3DController.changeSceneVisibility(checked)
            }
        }

        ParamGroup {
            groupName: "Bottom track"
            Layout.margins: 10
            spacing: 24


            CCheck {
                id: displayTrackCheckBox
                Layout.fillWidth: true
                checked: Scene3DModel.bottomTrackVisibility()
                text: "Display bottom track"
                onCheckedChanged: Settings3DController.changeBottomTrackVisibility(checked)
            }

            ParamGroup {
                groupName: "Surface"
                Layout.margins: 10
                spacing: 24

                ParamSetup {
                    paramName: "Stage:"
                    CCombo  {
                        id: stageCCombo
                        Layout.fillWidth: true
                        model: ["TIN", "UGIN"]
                        enabled:Scene3DModel.triangulationAvailable()
                        currentIndex: 1
                        onCurrentTextChanged: Settings3DController.chageDisplayedStage(currentText)

                        contentItem: Text {
                            font: theme.textFont
                            text: stageCCombo.currentText
                            color: enabled ? theme.textColor : theme.disabledTextColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            color: {
                                !stageCCombo.enabled ? theme.disabledBackColor :
                                 stageCCombo.hovered ? theme.hoveredBackColor  :
                                                       theme.controlBackColor
                            }
                        }
                    }
                }

                CCheck {
                    id: displaySurfaceCheckBox
                    Layout.fillWidth: true
                    checked: Scene3DModel.surfaceVisibility()
                    text: "Display surface"
                    onCheckedChanged: Settings3DController.changeSurfaceVisibility(checked)
                }

                CCheck {
                    id: displayGridCheckBox
                    Layout.fillWidth: true
                    checked: Scene3DModel.surfaceGridVisibility()
                    text: "Display grid"
                    onCheckedChanged: Settings3DController.changeSurfaceGridVisibility(checked)
                }

                ParamSetup {
                    paramName: "Interpolation level:"

                    CCombo  {
                        id: interpLevelCCombo
                        Layout.fillWidth: true
                        model: ["1", "2", "3"]
                        currentIndex: 0
                        enabled: (objectTypeCCombo.currentText !== "Track") && (Scene3DModel.triangulationAvailable())
                        onCurrentTextChanged: Settings3DController.setInterpolationLevel(currentText)

                        contentItem: Text {
                            font: theme.textFont
                            text: interpLevelCCombo.currentText
                            color: enabled ? theme.textColor : theme.disabledTextColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            color: {
                                !interpLevelCCombo.enabled ? theme.disabledBackColor :
                                 interpLevelCCombo.hovered ? theme.hoveredBackColor  :
                                                             theme.controlBackColor
                            }
                        }
                    }

                }

                ParamSetup {
                    paramName: "Triangulation edge length limit:"

                    SpinBoxCustom {
                        id: maxTriEdgeLengthSpinBox
                        from: 0
                        to: 100
                        stepSize: 2
                        value: 10
                        onValueChanged: Settings3DController.changeMaxTriangulationLineLength(value)
                    }

                }
            }


            CButton {
                id: updateSurfaceButton
                Layout.fillWidth: true          
                enabled: (objectTypeCCombo.currentText !== "Track") && (Scene3DModel.triangulationAvailable())
                onClicked: {
                    if (active)
                        Settings3DController.updateDisplayedObject()                   
                }

                contentItem: Text {
                    font: theme.textFont
                    text: "Update surface"
                    color: enabled ? theme.textColor : theme.disabledTextColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    color: {
                        !updateSurfaceButton.enabled ? theme.disabledBackColor :
                         updateSurfaceButton.hovered ? theme.hoveredBackColor  :
                                                       theme.controlBackColor
                    }
                }

            }
        }

    }

    Connections {
        target: Scene3DModel
        onStateChanged: {
            displayTrackCheckBox.enabled     = Scene3DModel.triangulationAvailable()
            displaySurfaceCheckBox.enabled   = Scene3DModel.triangulationAvailable()
            displayGridCheckBox.enabled      = Scene3DModel.triangulationAvailable()
            interpLevelCCombo.enabled        = Scene3DModel.triangulationAvailable() && stageCCombo.currentText != "TIN"
            updateSurfaceButton.enabled      = Scene3DModel.triangulationAvailable()
            maxTriEdgeLengthSpinBox.enabled  = Scene3DModel.triangulationAvailable()
            stageCCombo.enabled              = Scene3DModel.triangulationAvailable()
            maxTriEdgeLengthSpinBox.from     = Scene3DModel.minTriEdgeLength()
            maxTriEdgeLengthSpinBox.to       = Scene3DModel.maxTriEdgeLength()
            maxTriEdgeLengthSpinBox.stepSize = Math.abs(maxTriEdgeLengthSpinBox.to-maxTriEdgeLengthSpinBox.from) / 20;
        }
    }
}
