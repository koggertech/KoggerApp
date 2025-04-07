import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

import KoggerCommon 1.0

Item {
    readonly property var controller : NpdFilterControlMenuController

    function setFilter(filter){
        if(!filter)
            return

        distanceValueSpinBox.signalsBlocked = true
        distanceValueSpinBox.value          = filter.distance
        distanceValueSpinBox.signalsBlocked = false
    }

    id:         root
    objectName: "npdFilterControlMenu"
    height:     distanceValueSpinBox.height

    KParamSetup {
        anchors.fill: root
        paramName:    qsTr("Distance: ")

        KSpinBox {
            property bool signalsBlocked : false

            id:         distanceValueSpinBox
            objectName: "distanceValueSpinBox"
            from:       1
            to:         10000
            value:      1
        }

    }
}
