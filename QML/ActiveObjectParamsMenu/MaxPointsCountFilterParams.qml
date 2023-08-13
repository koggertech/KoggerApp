import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

import KoggerCommon 1.0

Item {
    id: root

    readonly property var filter : null

    function setFilter(filter){
        if(filter === null)
            return

        pointsCountSpinBox.value = filter.maxPointsCount()
    }

    Layout.fillWidth: true

    KParamSetup {
        anchors.top:   root.top
        anchors.left:  root.left
        anchors.right: root.right

        paramName: "Points count: "

        KSpinBox {
            id: pointsCountSpinBox
            from: 1
            to: 10000
            value: 100
            onValueChanged: MPCFilterParamsController.setMaxPointsCount(value)
        }

    }
}
