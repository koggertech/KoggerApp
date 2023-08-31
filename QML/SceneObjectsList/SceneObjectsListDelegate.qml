import QtQuick 2.15
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Component{
    id: sceneObjectListItemDelegate

    RowLayout{

        readonly property ListView listView: ListView.view
        readonly property string name:       model.name
        readonly property string type:       model.type
        readonly property string id:         model.id

        signal removeButtonClicked(index: int)

        anchors.left:  parent.left
        anchors.right: parent.right
        height:        40
        spacing:       0

        Rectangle {
            height:          parent.height
            color:           listView.currentIndex == index ? theme.controlBorderColor : theme.menuBackColor
            border.color:    "#919191"
            Layout.fillWidth: true

            Column {
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin:     18

                Text {
                    font.pixelSize: 14
                    text:           '<b>Name:</b> ' + model.display
                    color:          theme.textColor
                }
                //Text {
                //    font.pixelSize: 12
                //    text:           '<b>Type:</b> ' + model.type
                //    color:          theme.textColor
                //}
            }

            MouseArea{
                anchors.fill:    parent
                onClicked:       listView.currentIndex = index
                onDoubleClicked: {
                    textEditRectangle.visible = true
                    textEditRectangle.focus = true
                }
            }

            Rectangle{
                id:           textEditRectangle
                color:        "white"
                visible:      false
                anchors.fill: parent

                TextEdit {
                    id:             nameTextEdit
                    anchors.fill:   parent
                    font.pixelSize: 14
                    focus:          true

                    Keys.onPressed: {
                            if (event.key == Qt.Key_Return) {
                                textEditRectangle.visible = false
                                SceneObjectsListController.setObjectName(index, nameTextEdit.getText(0,10))
                                event.accepted = true;
                            }else{
                                event.accept()
                            }
                    }
                }
            }
        }

        Button{
            Layout.maximumWidth:  parent.height
            Layout.maximumHeight: parent.height

            Rectangle{
                anchors.fill: parent
                color:        "#990000"
                border.color: "#919191"

                Text{
                    anchors.centerIn: parent
                    text:             "X"
                    font.pixelSize:   16
                    color:            "#EEEEEF"
                }
            }

            onClicked: SceneObjectsListController.removeObject(index)
        }
    }
}
