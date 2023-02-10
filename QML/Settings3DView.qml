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
                    model: ["Track", "Surface", "Mesh",]
                    currentIndex: 0

                    onCurrentTextChanged: Settings3DController.chageDisplayedObjectType(currentText)
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
        }
    }
}
