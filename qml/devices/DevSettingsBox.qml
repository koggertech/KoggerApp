import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../controls"
import "../menus"

GridLayout {
    property var dev: null
    property bool isActive: false
    visible: dev !== null && isActive
}
