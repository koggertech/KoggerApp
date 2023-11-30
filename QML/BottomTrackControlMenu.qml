import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

import KoggerCommon 1.0
Item {
    readonly property var controller : BottomTrackControlMenuController

    id:         root
    objectName: "bottomTrackControlMenu"

    MenuBlockEx {
        id:           menuBlock
        anchors.fill: parent

        KParamGroup {
            id: paramGroup
            anchors.left:    parent.left
            anchors.right:   parent.right
            anchors.top:     parent.top
            anchors.margins: 24
            spacing:         24
            groupName:       "Bottom track controls"

            KCheck {
                id:               visibilityCheckBox
                text:             "Show"
                checkState:       Qt.Checked
                onCheckedChanged: root.controller.onVisibilityCheckBoxCheckedChanged(checked)
                Component.onCompleted: {
                    if(controller.bottomTrack)
                        checkState = controller.bottomTrack.visible ? Qt.Checked : Qt.Unchecked
                }
            }

            KButton {
                id:               restoreBottomTrack
                Layout.fillWidth: true
                text:             qsTr("Restore")
                onClicked:        root.controller.onRestoreBottomTrackButtonClicked()
            }
        }
    }
}
