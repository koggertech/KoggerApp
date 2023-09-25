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

        pointsCountSpinBox.blocked = true
        pointsCountSpinBox.value   = filter.maxPointsCount
        pointsCountSpinBox.blocked = false
    }

    id: root
    Layout.fillWidth: true

    KParamSetup {
        anchors.top:   root.top
        anchors.left:  root.left
        anchors.right: root.right
        paramName:     qsTr("Points count: ")

        KSpinBox {
            property bool blocked : false

            id:    pointsCountSpinBox
            from:  1
            to:    10000
            value: 100
            onValueChanged: {
                if(!blocked)
                   root.controller.onPointsCountSpinBoxValueChanged(value)
            }
        }

    }
}
