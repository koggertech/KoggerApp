import QtQuick 2.15
import kqml_types 1.0

Column {
    id: root

    required property var store
    required property int leafId

    width: parent ? parent.width : implicitWidth
    spacing: 10

    KSwitch {
        width: root.width
        text: qsTr("Rotate logo")
        checked: root.store.paneRotate3DByLeafId(root.leafId)
        onToggled: {
            root.store.setPaneRotate3DByLeafId(root.leafId, checked)
        }
    }
}
