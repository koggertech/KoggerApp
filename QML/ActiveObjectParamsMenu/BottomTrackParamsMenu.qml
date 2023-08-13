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

        if(activeObject === null)
            return

        visibilityCheckBox.checked = activeObject.visible

        initFilterTypeComboBox()
        initFilterParamsComponent()
    }

    function initFilterTypeComboBox(){
        var filter = activeObject.filter

        if(filter === null){
            filterTypeCombo.currentIndex = 0
            return
        }

        if(filter.type() === 1){
            filterTypeCombo.currentIndex = 1
        }else if(filter.type() === 2){
            filterTypeCombo.currentIndex = 2
        }else{
            filterTypeCombo.currentIndex = 0
        }
    }

    function initFilterParamsComponent(){
        var filter = activeObject.filter

        print("Id: " + activeObject.id)

        if(filter === null){
            filterParamsLoader.sourceComponent = filterParamsPlaceholder
            return
        }
        if(filter.type() === 1){
            filterParamsLoader.source = "MaxPointsCountFilterParams.qml"
        }else if(filter.type() === 2){
            filterParamsLoader.source = "NearestPointDistanceFilterParams.qml"
        }else{
            filterParamsLoader.sourceComponent = filterParamsPlaceholder
            return;
        }

        filterParamsLoader.item.setFilter(filter);
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
                onCheckedChanged: BottomTrackParamsController.changeBottomTrackVisibility(checked)
            }

            KParamSetup {
                paramName: "Filter: "

                KCombo {
                    id:          filterTypeCombo
                    model:       ["None", "Max points count", "Nearest point distance"]
                    onActivated: BottomTrackParamsController.changeBottomTrackFilter(index)
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

    Connections {
        target: activeObject
        onFilterChanged: {
            initFilterParamsComponent()
        }

    }
}
