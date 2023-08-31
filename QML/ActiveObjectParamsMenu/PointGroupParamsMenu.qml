import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

import SceneObject 1.0
import KoggerCommon 1.0

Item {
    id: root

    property var activeObject: null

    function setActiveObject(object){
        activeObject = object

        if (!object)
            return

        visibilityCheckBox.checked = activeObject.visible
    }

    Layout.minimumWidth:  160
    Layout.minimumHeight: 240

    MenuBlockEx {
        anchors.fill: parent

        KParamGroup {
            id:              paramGroup
            anchors.left:    parent.left
            anchors.right:   parent.right
            anchors.top:     parent.top
            anchors.margins: 24
            spacing:         10
            groupName:       activeObject ? activeObject.name : ""

            ColumnLayout {
                spacing: 4

                KCheck {
                    id:               visibilityCheckBox
                    text:             "Show"
                    onCheckedChanged: PointGroupParamsController.changePointSetVisibility(checked)
                }

                Rectangle {
                    Layout.fillWidth:     true
                    Layout.minimumWidth:  200
                    Layout.minimumHeight: 120
                    color:                "transparent"
                    border.color:         theme.controlBorderColor

                    ListView {
                        id:           pointListView
                        model:        activeObject.model
                        anchors.fill: parent

                        delegate: Rectangle {
                            anchors.left:     parent.left
                            anchors.right:    parent.right
                            color:            pointListView.currentIndex === index ? theme.controlBorderColor : theme.menuBackColor
                            border.color:     "#919191"
                            Layout.fillWidth: true
                            height: 20

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                font.pixelSize:         14
                                text:                   model.display
                                color:                  theme.textColor
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked:    {
                                    if(pointListView.currentIndex !== index)
                                        pointListView.currentIndex = index
                                }
                            }
                        }

                        onCurrentIndexChanged: {
                            pointObject = activeObject.pointAt(currentIndex)

                            pointXSpinBox.value = pointObject.x();
                            pointYSpinBox.value = pointObject.y();
                            pointZSpinBox.value = pointObject.z();
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    KButton {
                        Layout.fillWidth: true
                        text:             "Add point"
                        onClicked:        PointGroupParamsController.addPolygon()
                    }

                    KButton {
                        Layout.fillWidth: true
                        text:             "Remove point"
                        onClicked:        PointGroupParamsController.removePoint(pointListView.currentIndex)
                    }
                }

                ColumnLayout {
                    id: pointControlLayout
                    spacing: 0
                    visible: pointListView.count !== 0

                    function updatePointCoord(){
                        PointGroupParamsController.updatePointCoord(pointListView.currentIndex,
                                                                    Qt.vector3d(currentPointXSpinBox.value,
                                                                                currentPointYSpinBox.value,
                                                                                currentPointZSpinBox.value));
                    }

                    KParamSetup {
                        id: pointXSpinBox
                        paramName: "x: "

                        KSpinBox {
                            onValueChanged: pointControlLayout.updatePointCoord()
                        }
                    }

                    KParamSetup {
                        id: pointYSpinBox
                        paramName: "y: "

                        KSpinBox {
                            onValueChanged: pointControlLayout.updatePointCoord()
                        }
                    }

                    KParamSetup {
                    id: pointZSpinBox
                    paramName: "z: "

                        KSpinBox {
                            onValueChanged: pointControlLayout.updatePointCoord()
                        }
                    }
                }
            }
        }
    }

    //Connections {
    //    target: activeObject
    //    onCountChanged: {
    //        polygonListView.model = activeObject.visualItems()
    //    }
    //}
}
