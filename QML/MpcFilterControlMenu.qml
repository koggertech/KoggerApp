import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

import KoggerCommon 1.0

Item {
    readonly property var controller : MpcFilterControlMenuController

    function setFilter(filter){
        if(!filter)
            return

        pointsCountSpinBox.signalsBlocked = true
        pointsCountSpinBox.value          = filter.maxPointsCount
        pointsCountSpinBox.signalsBlocked = false
    }

    id:         root
    objectName: "mpcFilterControlMenu"
    height:     pointsCountSpinBox.height

    KParamSetup {
        anchors.fill: root
        paramName:    qsTr("Points count: ")

        KSpinBox {
            property bool signalsBlocked : false

            id:         pointsCountSpinBox
            objectName: "pointsCountSpinBox"
            from:       1
            to:         1000000
            value:      100
        }
    }
}
