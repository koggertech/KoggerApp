import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQml.Models 2.15
import QtCore
import "../controls"


Rectangle {
    id: consoleOut
    width: parent.width
    height: parent.height
    color: theme.menuBackColor

    ColumnLayout {
        width: parent.width
        height: parent.height



        Rectangle {
            Layout.fillWidth: true
            height: 36
            color: theme.menuBackColor

            RowLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                CCheck {
                    id: consScrollEnable
                    checked: true
                    text: qsTr("Auto scroll")
                    Layout.alignment: Qt.AlignVCenter

                    Settings {
                        property alias consScrollEnable: consScrollEnable.checked
                    }
                }

                CCheck {
                    id: protoBinConsoled
                    checked: false
                    text: qsTr("Binnary")
                    Layout.alignment: Qt.AlignVCenter

                    onCheckedChanged: deviceManagerWrapper.setProtoBinConsoled(protoBinConsoled.checked)
                    Component.onCompleted: deviceManagerWrapper.setProtoBinConsoled(protoBinConsoled.checked)

                    Settings {
                        property alias protoBinConsoled: protoBinConsoled.checked
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                CheckButton {
                    checkable: false
                    iconSource: "qrc:/icons/ui/x.svg"
                    Layout.alignment: Qt.AlignVCenter
                    implicitWidth: theme.controlHeight
                    implicitHeight: theme.controlHeight
                    onClicked: theme.consoleVisible = false

                    CMouseOpacityArea {
                        toolTipText: qsTr("Close console")
                        popupPosition: "bottomLeft"
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            DelegateModel {
                id: visualModel

                model: core.consoleList

                groups: [
                    DelegateModelGroup { name: "selected" }
                ]

                delegate: RowLayout {

                    TextEdit  {
                        Layout.fillWidth: true
                        text: time + "  " + payload
                        height: 16
                        font.pointSize: 10
                        font.family: "Console"
                        color: theme.textColor
                        readOnly: true
                        selectByMouse: true
                    }
                }
            }

            ListView {
                model: visualModel
                Layout.margins: 12
                Layout.fillWidth: true
                Layout.fillHeight: true

                onCountChanged: {
                    if(consScrollEnable.checked) {
                        Qt.callLater( positionViewAtEnd )
                    }
                }

                ScrollBar.vertical: ScrollBar { }

                focus: true

            }
        }
    }
}
