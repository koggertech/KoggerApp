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
    }

    Layout.minimumWidth:  160
    Layout.minimumHeight: 240

    MenuBlockEx {
        anchors.fill: parent

        Column {
            anchors.centerIn: parent

            Text {
                text: "Object not choosed"
                color: theme.textColor
            }
        }
    }
}
