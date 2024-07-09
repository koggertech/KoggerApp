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

    Component.onCompleted: sceneMenuButton.checked = true

    Item {
        id: menuLoader
        objectName:          "activeObjectParamsMenuLoader"
        Layout.minimumWidth:  440
        Layout.minimumHeight: 640

        Loader {
            objectName:   "loader"
            id:           loader
            anchors.fill: parent
            //source:       "ActiveObjectParamsMenu/BottomTrackParamsMenu.qml"
        }
    }

    RowLayout{
        id:            buttonGroupLayout
        Layout.alignment: Qt.AlignLeft
        Layout.fillWidth: true
        spacing:       0

        // Button{
        //     id:               sceneMenuButton
        //     Layout.fillWidth: true
        //     checkable:        true
        //     text:             "Scene"
        //     Layout.maximumWidth: menuLoader.width/buttonGroup.buttons.length

        //     onCheckedChanged: {
        //         if(checked)
        //             loader.source = "Scene3DControlMenu.qml"
        //     }
        // }

        Button{
            id:               bottomTrackMenuButton
            Layout.fillWidth: true
            checkable:        true
            text:             qsTr("Bottom track")
            Layout.maximumWidth: menuLoader.width/buttonGroup.buttons.length

            onCheckedChanged: {
                if(checked)
                    loader.source = "BottomTrackControlMenu.qml"
            }
        }

        Button{
            Layout.fillWidth: true
            checkable:        true
            text:             qsTr("Surface")
            Layout.maximumWidth: menuLoader.width/buttonGroup.buttons.length
            Component.onCompleted: checked = true

            onCheckedChanged: {
                if(checked)
                    loader.source = "SurfaceControlMenu.qml"
            }
        }

        Button{
            Layout.fillWidth: true
            checkable:        true
            text:             qsTr("Point group")
            Layout.maximumWidth: menuLoader.width/buttonGroup.buttons.length

            onCheckedChanged: {
                if(checked)
                    loader.source = "PointGroupControlMenu.qml"
            }
        }

        Button{
            Layout.fillWidth: true
            checkable:        true
            text:             qsTr("Polygon group")
            Layout.maximumWidth: menuLoader.width/buttonGroup.buttons.length

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

