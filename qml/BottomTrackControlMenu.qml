import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs

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
            groupName:       qsTr("Bottom track controls")

            KCheck {
                id:               visibilityCheckBox
                text:             qsTr("Show")
                checkState:       Qt.Checked
                onCheckedChanged: root.controller.onVisibilityCheckBoxCheckedChanged(checked)

                Component.onCompleted: {
                    if(root.controller.bottomTrack)
                        visibilityCheckBox.checkState = controller.bottomTrack.visible ? Qt.Checked : Qt.Unchecked
                }
            }

            KParamSetup {
                paramName: "Visible channel: "

                KCombo {
                    id:                    visibleChannelCombo
                    objectName:            "visibleChannelCombo"
                    Layout.preferredWidth: 250
                    model:                 root.controller.channelList
                    onActivated:           root.controller.onVisibleChannelComboBoxIndexChanged(currentIndex)

                    Component.onCompleted: {
                        visibleChannelCombo.currentIndex = controller.visibleChannelIndex
                    }
                }
            }
        }
    }

    Connections {
        target: root.controller
        function onChannelListUpdated() {
            visibleChannelCombo.currentIndex = controller.visibleChannelIndex
            visibleChannelCombo.model = controller.channelList
        }
    }
}
