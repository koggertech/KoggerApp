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

    id:               root
    Layout.fillWidth: true

    KParamSetup {
        anchors.top:   root.top
        anchors.left:  root.left
        anchors.right: root.right

        paramName: qsTr("Distance: ")

        KSpinBox {
            property bool signalsBlocked : false

            id:    distanceValueSpinBox
            from:  1
            to:    10000
            value: 1
            onValueChanged: {
                if(!signalsBlocked)
                    root.controller.onDistanceSpinBoxValueChanged(value)
            }
        }

    }
}
