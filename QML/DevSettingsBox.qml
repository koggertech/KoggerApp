import QtQuick 2.0

Item {
    property var dev: null
    property bool isActive: false
    visible: dev !== null && isActive
}
