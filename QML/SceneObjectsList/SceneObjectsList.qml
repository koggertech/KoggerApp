import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Qt3D.Render 2.15

import SceneObject 1.0

Rectangle{

    signal currentItemChanged(name: string, type: string, index: int)
    signal countChanged(count: int)
    signal itemCreated(type: string)

    readonly property real  controlsHeight: 40
    readonly property int itemsCount: sceneObjectsListView.count

    id: root
    Layout.minimumWidth:  200
    Layout.minimumHeight: 300
    color:                theme.menuBackColor

    ColumnLayout{
        anchors.fill: parent

        ListView{
            id:                   sceneObjectsListView
            Layout.fillWidth:     true
            Layout.fillHeight:    true
            model:                core.sceneItemListModel
            delegate:             SceneObjectsListDelegate{}
            focus:                true
            onCurrentItemChanged: {
                SceneObjectsListController.setCurrentObject(currentIndex)
                //root.currentItemChanged(currentItem.name, currentItem.type, currentIndex)
            }
            onCountChanged:       root.countChanged(count)
        }

        RowLayout{
            anchors.left:      parent.left
            anchors.right:     parent.right
            Layout.fillHeight: true

            Button{
                id:                     addObjectOnSceneButton
                Layout.preferredHeight: controlsHeight
                Layout.preferredWidth:  controlsHeight
                onClicked:              {
                    var objectType = SceneObject.Unknown

                    if(objectTypeComboBox.currentText === "Bottom track")
                        objectType = SceneObject.BottomTrack
                    if(objectTypeComboBox.currentText === "Surface")
                        objectType = SceneObject.Surface
                    else if(objectTypeComboBox.currentText === "Point group")
                        objectType = SceneObject.PointGroup
                    else if(objectTypeComboBox.currentText === "Polygon group")
                        objectType = SceneObject.PolygonGroup

                    SceneObjectsListController.addObject("Scene object", objectType)
                }

                background: Rectangle {
                    radius:       1
                    height:       parent.height
                    width:        parent.width
                    color:        addObjectOnSceneButton.pressed ? theme.controlBorderColor : theme.menuBackColor
                    border.color: theme.controlBorderColor
                    border.width: 1
                }

                Text{
                    anchors.centerIn: parent
                    text:             "+"
                    font.pixelSize:   16
                    font.bold:        true
                    color:            theme.textColor
                }
            }

            ComboBox{
                id:               objectTypeComboBox
                Layout.fillWidth: true
                height:           controlsHeight
                model:            ["Bottom track", "Surface", "Point group", "Polygon group", "Bottom track extended"]
                font.pixelSize:   16
                font.bold:        true

            }
        }
    }
}



