import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

import KoggerCommon 1.0
Item {
    readonly property var controller : BottomTrackControlMenuController

    id:                   root
    objectName:           "bottomTrackControlMenu"
    //Layout.minimumWidth:  160
    //Layout.minimumHeight: 240

    MenuBlockEx {
        id:           menuBlock
        anchors.fill: parent

        Component.onCompleted: {
            var bottomTrack = controller.bottomTrack
            if(!bottomTrack){
                visibilityCheckBox.checkState = Qt.Checked
                return;
            }

            visibilityCheckBox.checkState = bottomTrack.visible ? Qt.Checked : Qt.Unchecked
            filterTypeCombo.currentIndex  = bottomTrack.filter.type

            initFilterParamsMenu(bottomTrack.filter)
        }

        function initFilterParamsMenu(filter){
            if(!filter)
                filterParamsLoader.sourceComponent = filterParamsPlaceholder

            switch(filter.type){
            case 1: filterParamsLoader.source = "MpcFilterControlMenu.qml"
                break
            case 2: filterParamsLoader.source = "NpdFilterControlMenu.qml"
                break
            }

            filterParamsLoader.item.setFilter(filter)
        }


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
                onCheckedChanged: root.controller.onVisibilityCheckBoxCheckedChanged(checked)
            }

            KParamSetup {
                paramName: "Filter: "

                KCombo {
                    id:          filterTypeCombo
                    model:       ["None", "Max points count", "Nearest point distance"]
                    onActivated: {
                        root.controller.onFilterTypeComboBoxIndexChanged(index)
                        menuBlock.initFilterParamsMenu(controller.bottomTrack.filter)
                    }
                }
            }

            Loader {
                id:               filterParamsLoader
                Layout.fillWidth: true
                sourceComponent:  filterParamsPlaceholder
            }

            Component {
                 id: filterParamsPlaceholder
                 Item {}
            }
        }
    }
}
