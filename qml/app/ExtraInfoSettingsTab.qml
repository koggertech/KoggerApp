import QtQuick 2.15
import kqml_types 1.0

Column {
    id: page

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceMd

    KSwitch {
        width: parent.width
        text: qsTr("Show extra info panel")
        checked: page.store ? page.store.extraInfoVisible : false
        onToggled: if (page.store) page.store.extraInfoVisible = checked
    }

    Text {
        text: qsTr("Fields:")
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontXs
        leftPadding: Tokens.spaceXxs
    }

    KSwitch {
        width: parent.width; text: qsTr("Depth")
        checked: page.store ? page.store.extraInfoDepth : true
        onToggled: if (page.store) page.store.extraInfoDepth = checked
    }
    KSwitch {
        width: parent.width; text: qsTr("Speed")
        checked: page.store ? page.store.extraInfoSpeed : true
        onToggled: if (page.store) page.store.extraInfoSpeed = checked
    }
    KSwitch {
        width: parent.width; text: qsTr("Coordinates")
        checked: page.store ? page.store.extraInfoCoordinates : true
        onToggled: if (page.store) page.store.extraInfoCoordinates = checked
    }
    KSwitch {
        width: parent.width; text: qsTr("Active point")
        checked: page.store ? page.store.extraInfoActivePoint : true
        onToggled: if (page.store) page.store.extraInfoActivePoint = checked
    }
    KSwitch {
        width: parent.width; text: qsTr("Navigation info")
        checked: page.store ? page.store.extraInfoNav : false
        onToggled: if (page.store) page.store.extraInfoNav = checked
    }
    KSwitch {
        width: parent.width; text: qsTr("Boat status")
        checked: page.store ? page.store.extraInfoBoatStatus : false
        onToggled: if (page.store) page.store.extraInfoBoatStatus = checked
    }
}
