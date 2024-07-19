import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1
import QtQuick.Controls 1.4

import KoggerCommon 1.0

Item {
    readonly property var controller : PolygonGroupControlMenuController

    id:                   root
    objectName:           "polygonGroupControlMenu"
    Layout.minimumWidth:  160
    Layout.minimumHeight: 240

    MenuBlockEx {
        id:           menuBlock
        anchors.fill: parent

        Component.onCompleted: {
            visibilityCheckBox.checkState = root.controller.polygonGroup.visible ? Qt.Checked : Qt.Unchecked
            selectedPointParamGroup.visible = false
        }

        KParamGroup {
            id: paramGroup
            groupName:       qsTr("Polygon group controls")
            anchors.left:    parent.left
            anchors.right:   parent.right
            anchors.top:     parent.top
            anchors.margins: 24
            spacing:         24

            KCheck {
                id:               visibilityCheckBox
                text:             qsTr("Show")
                onCheckedChanged: root.controller.onVisibilityCheckBoxCheckedChanged(checked)
            }

            Rectangle {
                id: polygonListViewRectangle
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth:  200
                Layout.minimumHeight: 300
                color: "transparent"

                TreeView {
                    id:              polygonTreeView
                    model:           root.controller.polygonListModel
                    anchors.fill:    polygonListViewRectangle
                    anchors.margins: 2

                    TableViewColumn {
                        role: "display"
                        width: polygonTreeView.width
                    }

                    itemDelegate: Rectangle{
                        id:           polygonTreeViewItemDelegateRect
                        color:        "transparent"
                        width:        polygonTreeView.width

                        RowLayout {
                            id:           itemDelegateLayout
                            anchors.fill: polygonTreeViewItemDelegateRect

                            ColorDialog {
                                id:         polygonColorDialog
                                onAccepted: {
                                    root.controller.onPolygonColorDialogAccepted(polygonColorDialog.color, styleData.index)
                                }
                            }

                            Rectangle {
                                id:                   polygonColorPickRect
                                color:                model.color
                                Layout.maximumWidth:  itemDelegateLayout.height
                                Layout.maximumHeight: itemDelegateLayout.height
                                Layout.minimumWidth:  itemDelegateLayout.height
                                Layout.minimumHeight: itemDelegateLayout.height
                                border.color:         theme.controlBorderColor
                                visible:              styleData.depth === 0

                                MouseArea {
                                    anchors.fill: polygonColorPickRect
                                    onClicked:    polygonColorDialog.open()
                                }
                            }

                            Text {
                                color: theme.textColor
                                text:  model.display
                            }

                            Item {
                                id:               horizontalSpacer
                                Layout.fillWidth: true
                            }

                            Button {
                                id:                   addPointButton
                                Layout.maximumWidth:  itemDelegateLayout.height
                                Layout.maximumHeight: itemDelegateLayout.height
                                visible:              styleData.depth === 0
                                text:                 "+"
                                onClicked:            root.controller.onAddPointButtonClicked(styleData.index)
                            }

                            Button {
                                id:                   removeItemButton
                                Layout.maximumWidth:  itemDelegateLayout.height
                                Layout.maximumHeight: itemDelegateLayout.height
                                text:                 "x"
                                onClicked:            root.controller.onPolygonListItemRemoveButtonClicked(styleData.index)
                            }
                        }
                    }

                    onCurrentIndexChanged: {
                        if(currentIndex.parent === rootIndex){
                            selectedPointParamGroup.visible = false
                            return
                        }

                        if(!selectedPointParamGroup.visible)
                            selectedPointParamGroup.visible = true

                        var point = root.controller.pointAt(currentIndex)

                        selectedPointParamGroup.signalsBlocked = true
                        xValueSpinBox.value = point.position.x
                        yValueSpinBox.value = point.position.y
                        zValueSpinBox.value = point.position.z
                        selectedPointParamGroup.signalsBlocked = false
                    }
                }
            }


            KParamGroup {
                property bool signalsBlocked : false

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

                            root.controller.onPointCoordSpinBoxValueChanged(Qt.vector3d(value,
                                                                                                    yValueSpinBox.value,
                                                                                                    zValueSpinBox.value),
                                                                                        polygonTreeView.currentIndex)
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
                        onValueChanged: {
                            if(selectedPointParamGroup.signalsBlocked)
                                return

                            root.controller.onPointCoordSpinBoxValueChanged(Qt.vector3d(xValueSpinBox.value,
                                                                                                    value,
                                                                                                    zValueSpinBox.value),
                                                                                        polygonTreeView.currentIndex)
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
                        onValueChanged: {
                            if(selectedPointParamGroup.signalsBlocked)
                                return

                            root.controller.onPointCoordSpinBoxValueChanged(Qt.vector3d(xValueSpinBox.value,
                                                                                                    yValueSpinBox.value,
                                                                                                    value),
                                                                                        polygonTreeView.currentIndex)
                        }
                    }
                }
            }

            KButton {
                Layout.fillWidth: true
                text: qsTr("Add polygon")
                onClicked: root.controller.onAddPolygonButtonClicked()
            }
        }
    }
}
