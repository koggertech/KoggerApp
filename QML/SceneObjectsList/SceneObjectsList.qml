import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Qt3D.Render 2.15

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
            model:                SceneObjectListModel
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
                onClicked:              SceneObjectsListController.addObject("Scene object", objectTypeComboBox.currentText)

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
                model:            ["Bottom track", "Surface", "Point set", "Polygon set", "Bottom track extended"]
                font.pixelSize:   16
                font.bold:        true

            }
        }
    }
}



