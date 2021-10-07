import QtQuick 2.12
import QtQuick.Window 2.12

import QtQuick.Layouts 1.12

import QtQuick.Controls 2.12

import Qt.labs.settings 1.1
import QtQuick.Dialogs 1.2
import QtQuick.Controls 2.15

import WaterFall 1.0

Window  {
    id: mainview
    visible: true
    width: 1024
    minimumWidth: 512
    height: 512
    minimumHeight: 256
    color: "black"
    title: qsTr("KoggerApp, KOGGER")

    Settings {
        property alias x: mainview.x
        property alias y: mainview.y
        property alias width: mainview.width
        property alias height: mainview.height
    }

    SplitView {
        Layout.fillHeight: true
        Layout.fillWidth: true
        anchors.fill: parent

        orientation: Qt.Vertical
        visible: true

        handle: Rectangle {
            implicitWidth: 5
            implicitHeight: 5
            color: SplitHandle.pressed ? "#A0A0A0" : "#707070"

            Rectangle {
                width: parent.width
                height: 1
                color: "#A0A0A0"
            }

            Rectangle {
                y: parent.height
                width: parent.width
                height: 1
                color: "#A0A0A0"
            }
        }

        ColumnLayout {
            SplitView.fillHeight: true
            SplitView.fillWidth: true
            spacing: 0

            WaterFall {
                id: waterView
                visible: true
                width: mainview.width
                Layout.fillHeight: true
                Layout.fillWidth: true
                focus: true

                MouseArea {
                    id: mousearea



                    enabled: true
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onWheel: {
                        if (wheel.modifiers & Qt.ControlModifier) {
                            waterView.verZoomEvent(-wheel.angleDelta.y)
                        } else if (wheel.modifiers & Qt.ShiftModifier) {
                            waterView.verScrollEvent(-wheel.angleDelta.y)
                        } else {
                            waterView.horScrollEvent(wheel.angleDelta.y)
                        }
                    }

                    onClicked: {
                        waterView.focus = true
                        if (mouse.button === Qt.RightButton) {
                            contextMenu.popup()
                        }
                    }

                    onPressAndHold: contextMenu.popup()

                    Menu {
                        id: contextMenu
                        title: "Waterfall settings"

                        delegate: MenuItem {
                            id: menuItem
                            contentItem: CText {
                                text: menuItem.text
                            }

                            background: Rectangle {
                                implicitWidth: 300
                                height: theme.controlHeight
                                color: theme.menuBackColor
                                border.color: theme.controlBorderColor
                                border.width: 1
                                radius: 1
                            }

                            arrow: Canvas {
                                x: parent.width - width
                                implicitWidth: theme.controlHeight
                                implicitHeight: theme.controlHeight
                                visible: menuItem.subMenu
                                onPaint: {
                                    var ctx = getContext("2d")
                                    ctx.fillStyle = theme.textColor
                                    ctx.moveTo(10, 10)
                                    ctx.lineTo(width - 10, height / 2)
                                    ctx.lineTo(10, height - 10)
                                    ctx.closePath()
                                    ctx.fill()
                                }
                            }
                        }

                        background: Rectangle {
                            implicitWidth: 300
                            height: 9*theme.controlHeight
                            color: theme.menuBackColor
                            border.color: theme.controlBorderColor
                            border.width: 1
                            radius: 1
                        }



                        Menu {
                            id: popup_themes

                            title: "Echogram Themes"
                            font: theme.textFont


                            ListView {
                                id: popupThemeList
                                model: ["Blue", "Sepia", "WRGBD", "WhiteBlack", "BlackWhite"]
                                height: popupThemeList.count*theme.controlHeight
                                width: 300
                                delegate: CRadioDel {
                                    ButtonGroup.group: buttonGroup
                                    implicitWidth: popupThemeList.width
                                    text: modelData
                                    checked: index == popupThemeList.currentIndex
                                    onCheckedChanged: popupThemeList.currentIndex = index
                                }

                                ButtonGroup { id: buttonGroup }

                                onCurrentIndexChanged: plot.themeId = currentIndex
                                Component.onCompleted: plot.themeId = currentIndex

                                Settings {
                                    property alias waterfallThemeId: popupThemeList.currentIndex
                                }
                            }


                            background: Rectangle {
                                implicitWidth: popupThemeList.width
                                height: popupThemeList.count*theme.controlHeight
                                color: theme.menuBackColor
                                border.color: theme.controlBorderColor
                                border.width: 1
                                radius: 1
                            }
                        }

                        Menu {
                            id: echogramTypes

                            title: "Echogram Types"
                            font: theme.textFont


                            ListView {
                                id: echogramTypesList
                                model: ["Default", "Optimized"]
                                height: echogramTypesList.count*theme.controlHeight
                                width: 300
                                delegate: CRadioDel {
                                    ButtonGroup.group: echogramTypesGroup
                                    implicitWidth: popupThemeList.width
                                    text: modelData
                                    checked: index == echogramTypesList.currentIndex
                                    onCheckedChanged: echogramTypesList.currentIndex = index
                                }

                                ButtonGroup { id: echogramTypesGroup }

                                onCurrentIndexChanged: plot.imageType = currentIndex
                                Component.onCompleted: plot.imageType = currentIndex

                                Settings {
                                    property alias echogramTypesList: echogramTypesList.currentIndex
                                }
                            }


                            background: Rectangle {
                                implicitWidth: echogramTypesList.width
                                height: echogramTypesList.count*theme.controlHeight
                                color: theme.menuBackColor
                                border.color: theme.controlBorderColor
                                border.width: 1
                                radius: 1
                            }
                        }

                        ColumnLayout {
                            spacing: 0
                            CCheck {
                                id: echogramVisible
                                checked: true
                                text: "Echogram"
                                onCheckedChanged: plot.setChartVis(checked)
                                Component.onCompleted: plot.setChartVis(checked)
                            }

                            CCheck {
                                id: rangefinderVisible
                                text: "Rangefinder"
                                onCheckedChanged: plot.setDistVis(checked)
                                Component.onCompleted: plot.setDistVis(checked)
                            }

                            CCheck {
                                id: postProcVisible
                                text: "Post-Processing"
                                onCheckedChanged: plot.setDistProcVis(checked)
                                Component.onCompleted: plot.setDistProcVis(checked)
                            }

                            CCheck {
                                id: oscilVisible
                                text: "Oscilloscope"
                                onCheckedChanged: plot.setOscVis(checked)
                                Component.onCompleted: plot.setOscVis(checked)
                            }

                            CCheck {
                                id: ahrsVisible
                                text: "AHRS"
                                onCheckedChanged: plot.setAHRSVis(checked)
                                Component.onCompleted: plot.setAHRSVis(checked)
                            }

                            CCheck {
                                id: encoderVisible
                                text: "Encoders"
                                onCheckedChanged: plot.setEncoderVis(checked)
                                Component.onCompleted: plot.setEncoderVis(checked)
                            }

                            Settings {
                                property alias echogramVisible: echogramVisible.checked
                                property alias rangefinderVisible: rangefinderVisible.checked
                                property alias postProcVisible: postProcVisible.checked
                                property alias oscilVisible: oscilVisible.checked
                                property alias ahrsVisible: ahrsVisible.checked
                                property alias encoderVisible: encoderVisible.checked
                            }

                            CButton {
                                text: "Export"
                                Layout.fillWidth: true

                                FileDialog {
                                    id: exportFileDialog
                                    folder: shortcuts.home
                                    selectExisting: false
                                    nameFilters: ["(*.csv)", "(*.txt)", "(*.*)"]

                                    onAccepted: {
                                        core.exportPlotAsCVS(exportFileDialog.fileUrl.toString());
                                    }

                                    onRejected: { }
                                }

                                onClicked: exportFileDialog.open()
                            }

                        }

                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: theme.controlBorderColor
            }

            CSlider {
                id: historyScroll
                Layout.fillWidth: true
                width: mainview.width
                implicitHeight: theme.controlHeight

                stepSize: 0.0001
                from: 1
                to: 0

                onValueChanged: core.setTimelinePosition(value);
            }
        }

        Console {
            id: console_vis
            visible: theme.consoleVisible
            SplitView.minimumHeight: 100
        }
    }

    MenuBar {
        id: menuBar
        Layout.fillHeight: true
        height: waterView.height
    }
}
