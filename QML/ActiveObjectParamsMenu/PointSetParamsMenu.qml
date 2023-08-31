import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1
import KoggerCommon 1.0

Item {
    id: root

    property var activeObject: null
    property real previousContentY

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
            spacing:         24
            groupName:       activeObject.name

            KCheck {
                id:               visibilityCheckBox
                text:             "Show"
                onCheckedChanged: PointSetParamsController.changePointSetVisibility(checked)
            }

            Rectangle {
                Layout.fillWidth:     true
                Layout.minimumWidth:  200
                Layout.minimumHeight: 60
                color:                "transparent"
                border.color:         theme.controlBorderColor

                ListView {
                    id:           pointListView
                    anchors.fill: parent
                    model:        activeObject.model

                    delegate: Rectangle {
                        anchors.left:     parent.left
                        anchors.right:    parent.right
                        height:           20
                        color:            pointListView.currentIndex === index ? theme.controlBorderColor : theme.menuBackColor
                        border.color:     "#919191"
                        Layout.fillWidth: true

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            font.pixelSize:         12
                            text:                   '<b>X:</b> ' + model.x + '<b> Y:</b> ' + model.y + '<b> Z:</b> ' + model.z
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
                        var point = activeObject.points[pointListView.currentIndex]
                        //pointCoordXSpinBox.displayText = point.x
                        //pointCoordYSpinBox.displayText = point.y
                        //pointCoordZSpinBox.displayText = point.z
                    }
                }
            }


            ColumnLayout {
                Layout.fillWidth: true
                visible:          pointListView.count > 0

                RowLayout {
                    Layout.fillWidth: true

                    KText {
                        Layout.fillWidth: true
                        text:             "x: "
                        color:            theme.textColor
                    }

                    KSpinBox {
                        id:             pointCoordXSpinBox
                        value:          0
                        from:           -1000
                        to:             1000
                        onValueChanged: PointSetParamsController.changePointCoord(pointListView.currentIndex,
                                                                                  Qt.vector3d(value,
                                                                                              pointCoordYSpinBox.value,
                                                                                              pointCoordZSpinBox.value)
                                                                                )
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    KText {
                        Layout.fillWidth: true
                        text:             "y: "
                        color:            theme.textColor
                    }

                    KSpinBox {
                        id:    pointCoordYSpinBox
                        value: 0
                        from:  -1000
                        to:    1000
                        onValueChanged: PointSetParamsController.changePointCoord(pointListView.currentIndex,
                                                                                  Qt.vector3d(pointCoordXSpinBox.value,
                                                                                              value,
                                                                                              pointCoordZSpinBox.value)
                                                                                )
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    KText {
                        Layout.fillWidth: true
                        text:             "z: "
                        color:            theme.textColor
                    }

                    KSpinBox {
                        id:    pointCoordZSpinBox
                        value: 0
                        from:  -1000
                        to:    1000
                        onValueChanged: PointSetParamsController.changePointCoord(pointListView.currentIndex,
                                                                                  Qt.vector3d(pointCoordXSpinBox.value,
                                                                                              pointCoordYSpinBox.value,
                                                                                              value)
                                                                                )

                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true

                KButton {
                    Layout.fillWidth: true
                    text:             "Add point"
                    onClicked:        PointSetParamsController.addPoint(Qt.vector3d(0, 0, 0))
                }

                KButton {
                    Layout.fillWidth: true
                    text:             "Remove point"
                    onClicked:        PointSetParamsController.removePoint(pointListView.currentIndex)
                }
            }


        }
    }

    Connections {
        target: pointListView.model
        onModelAboutToBeReset: pointListView.previousContentY = pointListView.contentY
        onModelReset: pointListView.contentY = pointListView.previousContentY
    }
}
