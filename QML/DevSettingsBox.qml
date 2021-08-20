import QtQuick 2.12
import QtQuick.Layouts 1.3

GridLayout {
    property var dev: null
    property bool isActive: false
    visible: dev !== null && isActive
}
