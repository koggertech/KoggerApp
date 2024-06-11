import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

ColumnLayout {
    MenuFrame {
        id: surfaceSettings
        visible: updateSurface.hovered || rSurface.hovered
        Layout.alignment: Qt.AlignRight
        // Layout.fillWidth: true
        // width: 300

        CheckButton {
            id: rSurface
            implicitHeight: theme.controlHeight
            // implicitWidth: 300
            // width: 300
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor


            icon.source: "./icons/stack-backward.svg"
            // onCheckedChanged: Scene3dToolBarController.onBottomTrackVertexComboSelectionModeButtonChecked(checked)
        }
    }

    RowLayout {
        spacing: 2

        MenuButton {
            id: setCameraIsometricView
            width: theme.controlHeight
            height: theme.controlHeight
            icon.source: "./fit-in-view.svg"

            icon.color: theme.textColor

            onClicked: Scene3dToolBarController.onSetCameraMapViewButtonClicked()
        }

        MenuButton {
            id: fitAllinViewButton
            width: theme.controlHeight
            height: theme.controlHeight
            icon.source: "./icons/zoom-cancel.svg"


            icon.color: theme.textColor

            onClicked: Scene3dToolBarController.onFitAllInViewButtonClicked()
        }

        MenuButton {
            id: selectionToolButton
            width: theme.controlHeight
            height: theme.controlHeight
            checkable: true
            active: checked
            icon.source: "./icons/click.svg"
            icon.color: theme.textColor
            ButtonGroup.group: buttonGroup

            onCheckedChanged: Scene3dToolBarController.onBottomTrackVertexEditingModeButtonChecked(checked)
        }

        MenuButton {
            id: bottomTrackVertexComboSelectionToolButton
            width: theme.controlHeight
            height: theme.controlHeight
            checkable: true
            active: checked
            icon.source: "./combo-selection.svg"
            icon.color: theme.textColor
            ButtonGroup.group: buttonGroup

            onCheckedChanged: Scene3dToolBarController.onBottomTrackVertexComboSelectionModeButtonChecked(checked)
        }

        CheckButton {
            implicitHeight: theme.controlHeight
            implicitWidth: theme.controlHeight
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor

            icon.source: "./icons/route.svg"
            // icon.width: width
            // icon.height: height
        }

        CheckButton {
            implicitHeight: theme.controlHeight
            implicitWidth: theme.controlHeight
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor

            icon.source: "./icons/overline.svg"

            onCheckedChanged: {
                BottomTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
            }

            Component.onCompleted: {
                BottomTrackControlMenuController.onVisibilityCheckBoxCheckedChanged(checked)
            }
        }

        CheckButton {
            id: updateSurface
            implicitHeight: theme.controlHeight
            implicitWidth: theme.controlHeight
            backColor: theme.controlBackColor
            borderColor: theme.controlBackColor
            checkedBorderColor: theme.controlBorderColor
            hoverEnabled: true

            onCheckedChanged: {
                SurfaceControlMenuController.onSurfaceVisibilityCheckBoxCheckedChanged(checked)
            }

            Component.onCompleted: {
                SurfaceControlMenuController.onSurfaceVisibilityCheckBoxCheckedChanged(checked)
            }

            icon.source: "./icons/stack-backward.svg"
            // onCheckedChanged: Scene3dToolBarController.onBottomTrackVertexComboSelectionModeButtonChecked(checked)
        }


        ButtonGroup{
            property bool buttonChangeFlag : false
            id: buttonGroup
            onCheckedButtonChanged: buttonChangeFlag = true
            onClicked: {
                if(!buttonChangeFlag)
                    checkedButton = null

                buttonChangeFlag = false;
            }
        }
    }
}

// Rectangle {
//     // color: "#915181"

//     Row {
//         id: row

//         Button {
//             id: setCameraIsometricView
//             width: 40
//             height: 40
//             icon.source: "./3dcube.svg"

//             icon.color: theme.textColor

//             background: Rectangle {
//                 radius: 1
//                 height: parent.height
//                 width: parent.width
//                 color: parent.checked ? theme.controlBorderColor : theme.menuBackColor
//                 border.color: theme.controlBorderColor
//                 border.width: 1
//             }

//             onClicked: Scene3dToolBarController.onSetCameraIsometricViewButtonClicked()
//         }

//         Button {
//             id: fitAllinViewButton
//             width: 40
//             height: 40
//             icon.source: "./fit-in-view.svg"

//             icon.color: theme.textColor

//             background: Rectangle {
//                 radius: 1
//                 height: parent.height
//                 width: parent.width
//                 color: parent.checked ? theme.controlBorderColor : theme.menuBackColor
//                 border.color: theme.controlBorderColor
//                 border.width: 1
//             }

//             onClicked: Scene3dToolBarController.onFitAllInViewButtonClicked()
//         }

//         Row {
//             id: toolButtonsLayout
//             Button{
//                 id: selectionToolButton
//                 width: 40
//                 height: 40
//                 checkable: true
//                 icon.source: "./one-finger.svg"

//                 icon.color: theme.textColor

//                 background: Rectangle {
//                     radius: 1
//                     height: parent.height
//                     width: parent.width
//                     color: parent.checked ? theme.controlBorderColor : theme.menuBackColor
//                     border.color: theme.controlBorderColor
//                     border.width: 1
//                 }

//                 onCheckedChanged: Scene3dToolBarController.onBottomTrackVertexEditingModeButtonChecked(checked)
//             }

//             Button{
//                 id: bottomTrackVertexComboSelectionToolButton
//                 width: 40
//                 height: 40
//                 checkable: true
//                 icon.source: "./combo-selection.svg"
//                 icon.color: theme.textColor

//                 background: Rectangle {
//                     radius: 1
//                     height: parent.height
//                     width: parent.width
//                     color: parent.checked ? theme.controlBorderColor : theme.menuBackColor
//                     border.color: theme.controlBorderColor
//                     border.width: 1
//                 }

//                 onCheckedChanged: Scene3dToolBarController.onBottomTrackVertexComboSelectionModeButtonChecked(checked)
//             }
//         }
//     }


// }




