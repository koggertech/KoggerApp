import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

import KoggerCommon 1.0

Item {
    readonly property var controller : PointGroupControlMenuController

    id:                   root
    objectName:           "pointGroupControlMenu"
    Layout.minimumWidth:  160
    Layout.minimumHeight: 240

    MenuBlockEx {
        id:           menuBlock
        anchors.fill: parent

        Component.onCompleted: {
            visibilityCheckBox.checkState = root.controller.pointGroup.visible ? Qt.Checked : Qt.Unchecked

            if(pointList.count <= 0)
                selectedPointParamGroup.visible = false
        }

        KParamGroup {
            id:              paramGroup
            groupName:       "Point group controls"
            anchors.left:    parent.left
            anchors.right:   parent.right
            anchors.top:     parent.top
            anchors.margins: 24
            spacing:         24

            KCheck {
                id:               visibilityCheckBox
                text:             "Show"
                onCheckedChanged: root.controller.onVisibilityCheckBoxCheckedChanged(checked)
            }

            Rectangle {
                id: pointListViewRectangle
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth:  200
                Layout.minimumHeight: 300
                border.color: theme.controlBorderColor
                color: "transparent"

                ListView {
                    id:              pointList
                    model:           root.controller.pointListModel
                    anchors.fill:    pointListViewRectangle
                    anchors.margins: 2

                    delegate: Rectangle{
                        id:     delegateRect
                        height: 25
                        width:  pointList.width
                        color:  pointList.currentIndex === index ? theme.controlBorderColor : theme.menuBackColor

                        MouseArea{
                            anchors.fill: parent
                            onClicked:    pointList.currentIndex = index
                        }

                        RowLayout {
                            id: itemRowLayout
                            anchors.left:    delegateRect.left
                            anchors.right:   delegateRect.right
                            spacing:         6
                            anchors.margins: 10

                            ColorDialog {
                                id:         pointColorDialog
                                onAccepted: {
                                    root.controller.onPointColorDialogAccepted(pointColorDialog.color, index)
                                }
                            }

                            Rectangle {
                                id:                   pointColorPickRect
                                color:                model.color
                                Layout.maximumWidth:  delegateRect.height - itemRowLayout.spacing
                                Layout.maximumHeight: delegateRect.height - itemRowLayout.spacing
                                Layout.minimumWidth:  delegateRect.height - itemRowLayout.spacing
                                Layout.minimumHeight: delegateRect.height - itemRowLayout.spacing
                                border.color:         theme.controlBorderColor

                                MouseArea {
                                    anchors.fill: pointColorPickRect
                                    onClicked:    pointColorDialog.open()
                                }
                            }

                            Text {
                                font:                   theme.textFont
                                color:                  theme.textColor
                                text:                   model.display
                            }

                            Item {
                                id: spacerItem
                                Layout.fillWidth: true
                            }

                            Button {
                                id:                  removeItemButton
                                Layout.maximumWidth:  delegateRect.height - itemRowLayout.spacing
                                Layout.maximumHeight: delegateRect.height - itemRowLayout.spacing
                                Layout.minimumWidth:  delegateRect.height - itemRowLayout.spacing
                                Layout.minimumHeight: delegateRect.height - itemRowLayout.spacing

                                background: Rectangle{
                                    id:           removeItemButtonRect
                                    color:        "red"
                                    border.color: theme.controlBorderColor
                                    anchors.fill: removeItemButton

                                    Text {
                                        anchors.centerIn: removeItemButtonRect
                                        text:             "x"
                                        font.pixelSize:   14
                                        color:            theme.textColor
                                    }
                                }

                                onClicked: root.controller.onPointListItemRemoveButtonClicked(index)
                            }
                        }
                    }

                    onCountChanged: {
                        selectedPointParamGroup.fillSpinBoxValues(currentIndex)
                    }

                    onCurrentIndexChanged: {
                        selectedPointParamGroup.fillSpinBoxValues(currentIndex)
                    }
                }

                Text {
                    text: qsTr("Point list is empty. Add point to list or import list from file.")
                    color: theme.textColor
                    font.pixelSize: 14
                    visible: pointList.count <= 0
                    anchors.centerIn: pointListViewRectangle
                }

            }


            KParamGroup {
                property bool signalsBlocked : false

                function fillSpinBoxValues(row){
                    if(pointList.count <= 0){
                        selectedPointParamGroup.visible = false
                        return
                    }

                    if(!selectedPointParamGroup.visible)
                        selectedPointParamGroup.visible = true

                    var point = root.controller.pointAt(row)

                    selectedPointParamGroup.signalsBlocked = true
                    xValueSpinBox.value = point.position.x
                    yValueSpinBox.value = point.position.y
                    zValueSpinBox.value = point.position.z
                    widthSpinBox.value  = point.width
                    selectedPointParamGroup.signalsBlocked = false
                }

                id: selectedPointParamGroup
                groupName: qsTr("Selected point")

                KParamSetup {
                    paramName: "X: "

                    KSpinBox {
                        id: xValueSpinBox
                        from: -10000
                        to: 10000
                        value: 0
                        onValueChanged: {
                            if(selectedPointParamGroup.signalsBlocked)
                                return

                            root.controller.onCoordSpinBoxValueChanged(Qt.vector3d(value,
                                                                                               yValueSpinBox.value,
                                                                                               zValueSpinBox.value),
                                                                                   pointList.currentIndex)
                        }
                    }
                }

                KParamSetup {
                    paramName: "y: "

                    KSpinBox {
                        id: yValueSpinBox
                        from: -10000
                        to: 10000
                        value: 0
                        onValueChanged:  {
                            if(selectedPointParamGroup.signalsBlocked)
                                return

                            root.controller.onCoordSpinBoxValueChanged(Qt.vector3d(xValueSpinBox.value,
                                                                                               value,
                                                                                               zValueSpinBox.value),
                                                                                   pointList.currentIndex)
                        }
                    }
                }

                KParamSetup {
                    paramName: "z: "

                    KSpinBox {
                        id: zValueSpinBox
                        from: -10000
                        to: 10000
                        value: 0
                        onValueChanged:  {
                            if(selectedPointParamGroup.signalsBlocked)
                                return

                            root.controller.onCoordSpinBoxValueChanged(Qt.vector3d(xValueSpinBox.value,
                                                                                   yValueSpinBox.value,
                                                                                   value),
                                                                       pointList.currentIndex)
                        }
                    }
                }

                KParamSetup {
                    paramName: qsTr("width: ")

                    KSpinBox {
                        id: widthSpinBox
                        from: 1
                        to: 100
                        value: 6
                        onValueChanged:  {
                            if(selectedPointParamGroup.signalsBlocked)
                                return

                            root.controller.onPointWidthSpinBoxValueChanged(value, pointList.currentIndex)
                        }
                    }
                }


            }

            KButton {
                Layout.fillWidth: true
                text: qsTr("Add point")
                onClicked: root.controller.onAddPointButtonClicked()
            }
        }
    }
}
