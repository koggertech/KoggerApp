import QtQuick 2.15

Item {
    id: control

    property color backColor: "#505050"
    property real backOpacity: 0.9
    property color borderColor: "#595959"
    property real borderOpacity: 1.0
    property bool active: false
    property int ticknessCorner: 4
    property int ticknessBase: 2
    property int ticknessActive: 5
}
