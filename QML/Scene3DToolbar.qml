import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12


Item{

    Rectangle{
        color: "#915181"
        Row{
            id: row

            Button{
                id: selectionToolButton
                width: 40
                height: 40
                checkable: true
                icon.source: "./one-finger.svg"

                icon.color: theme.textColor

                background: Rectangle {
                    radius: 1
                    height: parent.height
                    width: parent.width
                    color: parent.checked ? theme.controlBorderColor : theme.menuBackColor
                    border.color: theme.controlBorderColor
                    border.width: 1
                }

                onClicked:{

                    if(checked){
                        buttonGroup.exclusive = false;
                        checked = false
                        buttonGroup.exclusive = true;
                    }else{
                        checked = true
                    }


                    Toolbar3dController.setSelectionToolState(checked)
                }
            }

            Button{
                width: 40
                height: 40
                checkable: true
                icon.source: "./tool.svg"

                icon.color: theme.textColor

                background: Rectangle {
                    radius: 1
                    height: parent.height
                    width: parent.width
                    color: parent.checked ? theme.controlBorderColor : theme.menuBackColor
                    border.color: theme.controlBorderColor
                    border.width: 1
                }

                //onClicked: Scene3DToolbarController.setSelectionToolState(checked)
            }

            Button{
                width: 40
                height: 40
                checkable: true
                icon.source: "./tool.svg"

                icon.color: theme.textColor

                background: Rectangle {
                    radius: 1
                    height: parent.height
                    width: parent.width
                    color: parent.checked ? theme.controlBorderColor : theme.menuBackColor
                    border.color: theme.controlBorderColor
                    border.width: 1
                }

                //onClicked: Scene3DToolbarController.setSelectionToolState(checked)
            }

            Button{
                width: 40
                height: 40
                checkable: true
                icon.source: "./tool.svg"

                icon.color: theme.textColor

                background: Rectangle {
                    radius: 1
                    height: parent.height
                    width: parent.width
                    color: parent.checked ? theme.controlBorderColor : theme.menuBackColor
                    border.color: theme.controlBorderColor
                    border.width: 1
                }

                //onClicked: Scene3DToolbarController.setSelectionToolState(checked)
            }

            Button{
                width: 40
                height: 40
                checkable: true
                icon.source: "./tool.svg"

                icon.color: theme.textColor

                background: Rectangle {
                    radius: 1
                    height: parent.height
                    width: parent.width
                    color: parent.checked ? theme.controlBorderColor : theme.menuBackColor
                    border.color: theme.controlBorderColor
                    border.width: 1
                }

                //onClicked: Scene3DToolbarController.setSelectionToolState(checked)
            }

        }

        ButtonGroup{
            id: buttonGroup
            buttons: row.children
            exclusive: true
        }
    }


}



