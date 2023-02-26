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
            groupName: "Displayed object"
            Layout.margins: 10
            spacing: 24

            ParamSetup {
                paramName: "Type:"

                CCombo  {
                    id: objectTypeCCombo
                    Layout.fillWidth: true
                    model: ["Track", "Surface with grid", "Surface", "Grid"]
                    enabled:Scene3DModel.triangulationAvailable()
                    currentIndex: 0

                    onCurrentTextChanged: Settings3DController.chageDisplayedObjectType(currentText)

                    contentItem: Text {
                        font: theme.textFont
                        text: objectTypeCCombo.currentText
                        color: enabled ? theme.textColor : theme.disabledTextColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        color: {
                            !objectTypeCCombo.enabled ? theme.disabledBackColor :
                             objectTypeCCombo.hovered ? theme.hoveredBackColor  :
                                                           theme.controlBackColor
                        }
                    }
                }

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
            updateSurfaceButton.enabled = (objectTypeCCombo.currentText !== "Track") && (Scene3DModel.triangulationAvailable())
            interpLevelCCombo.enabled   = (objectTypeCCombo.currentText !== "Track") && (Scene3DModel.triangulationAvailable())
            objectTypeCCombo.enabled    = (Scene3DModel.triangulationAvailable())
        }
    }
}
