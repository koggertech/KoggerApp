import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3

ColumnLayout {
    id: root

    function loadComponent(path : url){
        loader.setSource(path)
    }

    function reset(){
        loader.setSource(initialComponent)
    }

    Component.onCompleted: bottomTrackMenuButton.checked = true

    Item {
        objectName:          "activeObjectParamsMenuLoader"
        Layout.minimumWidth:  440
        Layout.minimumHeight: 640

        Loader {
            objectName:   "loader"
            id:           loader
            anchors.fill: parent
            source:       "ActiveObjectParamsMenu/BottomTrackParamsMenu.qml"
        }
    }

    RowLayout{
        id:            buttonGroupLayout
        anchors.left:  root.left
        anchors.right: root.right
        spacing:       0

        Button{
            id:               bottomTrackMenuButton
            Layout.fillWidth: true
            checkable:        true
            text:             "Bottom track"
            onCheckedChanged: {
                if(checked)
                    loader.source = "BottomTrackControlMenu.qml"
            }
        }

        Button{
            Layout.fillWidth: true
            checkable:        true
            text:             "Surface"
            onCheckedChanged: {
                if(checked)
                    loader.source = "SurfaceControlMenu.qml"
            }
        }

        Button{
            Layout.fillWidth: true
            checkable:        true
            text:             "Point group"
            onCheckedChanged: {
                if(checked)
                    loader.source = "PointGroupControlMenu.qml"
            }
        }

        Button{
            Layout.fillWidth: true
            checkable:        true
            text:             "Polygon group"
            onCheckedChanged: {
                if(checked)
                    loader.source = "PolygonGroupControlMenu.qml"
            }
        }
    }

    ButtonGroup{
        id:        buttonGroup
        buttons:   buttonGroupLayout.children
        exclusive: true
    }
}

