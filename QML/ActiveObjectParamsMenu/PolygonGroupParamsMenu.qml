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
    property var polygonObject: null

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
                    onCheckedChanged: PolygonGroupParamsController.changePointSetVisibility(checked)
                }

                Rectangle {
                    Layout.fillWidth:     true
                    Layout.minimumWidth:  200
                    Layout.minimumHeight: 120
                    color:                "transparent"
                    border.color:         theme.controlBorderColor

                    ListView {
                        id:           polygonListView
                        model:        activeObject.model
                        anchors.fill: parent

                        delegate: Rectangle {
                            anchors.left:     parent.left
                            anchors.right:    parent.right
                            color:            polygonListView.currentIndex === index ? theme.controlBorderColor : theme.menuBackColor
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
                                    if(polygonListView.currentIndex !== index)
                                        polygonListView.currentIndex = index
                                }
                            }
                        }

                        onCurrentIndexChanged: {
                            polygonObject = activeObject.polygonAt(currentIndex)
                            polygonPointListView.model = polygonObject.model
                        }
                        onCountChanged: {
                            if(count === 0){
                                polygonPointListView.model = []
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    KButton {
                        Layout.fillWidth: true
                        text:             "Add polygon"
                        onClicked:        PolygonGroupParamsController.addPolygon()
                    }

                    KButton {
                        Layout.fillWidth: true
                        text:             "Remove polygon"
                        onClicked:        PolygonGroupParamsController.removePolygon(polygonListView.currentIndex)
                    }
                }

                Rectangle {
                    Layout.fillWidth:     true
                    Layout.minimumWidth:  200
                    Layout.minimumHeight: 120
                    color:                "transparent"
                    border.color:         theme.controlBorderColor

                    ListView {
                        id:           polygonPointListView
                        model:        polygonObject.model
                        anchors.fill: parent

                        delegate: Rectangle {
                            anchors.left:     parent.left
                            anchors.right:    parent.right
                            color:            polygonPointListView.currentIndex === index ? theme.controlBorderColor : theme.menuBackColor
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
                                    if(polygonPointListView.currentIndex !== index)
                                        polygonPointListView.currentIndex = index
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    KButton {
                        Layout.fillWidth: true
                        text:             "Add point"
                        onClicked:        PolygonGroupParamsController.addPoint(polygonListView.currentIndex)
                    }

                    KButton {
                        Layout.fillWidth: true
                        text:             "Remove point"
                        onClicked:        PolygonGroupParamsController.removePoint(polygonListView.currentIndex, polygonPointListView.currentIndex)
                    }
                }


                ColumnLayout {
                    id: pointControlLayout
                    spacing: 0

                    function updatePointCoord(){
                        PolygonGroupParamsController.updatePointCoord(polygonListView.currentIndex,
                                                                      polygonPointListView.currentIndex,
                                                                      Qt.vector3d(currentPointXSpinBox.value,
                                                                                  currentPointYSpinBox.value,
                                                                                  currentPointZSpinBox.value));
                    }

                    KParamSetup {
                        id: currentPointXSpinBox
                        paramName: "x: "

                        KSpinBox {
                            onValueChanged: pointControlLayout.updatePointCoord()
                        }
                    }

                    KParamSetup {
                        id:currentPointYSpinBox
                        paramName: "y: "

                        KSpinBox {
                            onValueChanged: pointControlLayout.updatePointCoord()
                        }
                    }

                    KParamSetup {
                    id:currentPointZSpinBox
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
