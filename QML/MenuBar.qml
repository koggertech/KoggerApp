import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import Qt.labs.settings 1.1

Item {
    id: menu
    implicitWidth: menuLayout.width

    property var targetPlot: null

    property var lastItem: menuSettings
    property bool isConsoleVisible: consoleEnable.checked
    property bool is3DVisible: scene3DSettings.is3DVisible
    property bool is2DVisible: visible2dButton.checked
    property bool is2DHorizontal: appSettings.is2DHorizontal
    property int instruments:  appSettings.instruments
    property int settingsWidth: theme.controlHeight*20

    function itemChangeActive(currentItem) {
        if(currentItem !== null) {
            currentItem.active = !(currentItem.active)
        }

        if(lastItem !== null && lastItem !== currentItem) {
            lastItem.active = false
        }

        lastItem = currentItem
    }

    RowLayout {
        id: menuLayout
        Layout.fillWidth: true

        ColumnLayout {
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: theme.controlHeight*1.8
            Layout.topMargin: 10

            ColumnLayout {
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignHCenter

                MenuButton {
                    id: menuSettings
                    icon.source: "./code-working.svg"
                    Layout.fillWidth: true

                    onPressed: {
                        itemChangeActive(menuSettings)
                    }
                }

//                MenuButton {
//                    id: menuGraph
//                    Layout.fillWidth: true
////                    icon.source: "./settings-outline.svg"

//                    onPressed: {
//                        itemChangeActive(menuGraph)
//                    }
//                }

                MenuButton {
                    id: menuDisplay
                    Layout.fillWidth: true
                    icon.source: "./settings-outline.svg"

                    onPressed: {
                        itemChangeActive(menuDisplay)
                    }
                }

                MenuButton {
                    id: menu3DSettings
                    visible: instruments > 0
                    icon.source: "./3dcube.svg"

                    Layout.fillWidth: true

                    onPressed: {
                        itemChangeActive(menu3DSettings)
                    }
                }


            }

            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                visible: visible2dButton.checked

                MenuBlock {
                }


                CText {
                    Layout.fillWidth: true
                    Layout.topMargin: 4
                    visible: chartEnable.checked
                    horizontalAlignment: Text.AlignHCenter
                    text: echogramLevelsSlider.stopValue
                    small: true
                }

                ChartLevel {
                    Layout.fillWidth: true
                    id: echogramLevelsSlider
                    visible: chartEnable.checked
                    Layout.alignment: Qt.AlignHCenter

                    onStartValueChanged: {
                        targetPlot.plotEchogramSetLevels(startValue, stopValue);
                    }

                    onStopValueChanged: {
                        targetPlot.plotEchogramSetLevels(startValue, stopValue);
                    }

                    Component.onCompleted: {
                        targetPlot.plotEchogramSetLevels(startValue, stopValue);
                    }

                    Settings {
                        property alias echogramLevelsStart: echogramLevelsSlider.startValue
                        property alias echogramLevelsStop: echogramLevelsSlider.stopValue
                    }
                }

                CText {
                    Layout.fillWidth: true
                    Layout.bottomMargin: 4
                    visible: chartEnable.checked
                    horizontalAlignment: Text.AlignHCenter

                    text: echogramLevelsSlider.startValue
                    small: true
                }
            }

//            ColumnLayout {
//                Layout.alignment: Qt.AlignHCenter
//                spacing: 0
//                visible: visible2dButton.checked


//                MenuBlock { }

//                ButtonGroup { id: pencilbuttonGroup }

//                CButton {
//                    Layout.fillWidth: true
//                    Layout.margins: 4
//                    Layout.bottomMargin: 1
////                    Layout.preferredHeight: 24
//                    text: "⇔"
//                    checkable: true
//                    checked: true
//                    padding: 0
//                    onCheckedChanged: {
//                        if(checked) {  waterView.setMouseMode(1) }
//                    }
//                    ButtonGroup.group: pencilbuttonGroup
//                }

//                CButton {
//                    Layout.fillWidth: true
//                    Layout.margins: 4
//                    Layout.bottomMargin: 1
////                    Layout.preferredHeight: 24
//                    text: "⇲"
//                    checkable: true
//                    padding: 0
//                    onCheckedChanged: {
//                        if(checked) {  waterView.setMouseMode(2) }
//                    }
//                    ButtonGroup.group: pencilbuttonGroup
//                }

//                CButton {
//                    Layout.fillWidth: true
//                    Layout.margins: 4
//                    Layout.bottomMargin: 1
////                    Layout.preferredHeight: 24
//                    text: "═"
//                    checkable: true
//                    padding: 0
//                    onCheckedChanged: {
//                        if(checked) {  waterView.setMouseMode(3) }
//                    }
//                    ButtonGroup.group: pencilbuttonGroup
//                }

//                CButton {
//                    Layout.fillWidth: true
//                    Layout.margins: 4
////                    Layout.preferredHeight: 24
//                    text: "⇱"
//                    checkable: true
//                    padding: 0
//                    onCheckedChanged: {
//                        if(checked) {  waterView.setMouseMode(4) }
//                    }
//                    ButtonGroup.group: pencilbuttonGroup
//                }
//            }

            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 0
                visible: instruments > 0

                MenuBlock { }

                CButton {
                    id: settings3DButton
                    //id: visible3dButton
                    Layout.fillWidth: true
                    Layout.margins: 4
              //      Layout.preferredHeight: 24
                    text: "3D"
                    checkable: true
                    padding: 0

                    onClicked: {
                        Settings3DController.changeSceneVisibility(checked)
//                       itemChangeActive(settings3DButton)
                    }
                }

                CButton {
                    id: visible2dButton
                    Layout.fillWidth: true
                    Layout.margins: 4
//                    Layout.preferredHeight: 24
                    text: "2D"
                    checkable: true
                    checked: true
                    padding: 0
                    onClicked: {
//                        if(checked) { core.movePoints() }
                    }
                }
            }
        }



        DeviceSettingsViewer {
            id: devSettings
            Layout.alignment: Qt.AlignTop
            visible: menuSettings.active
            Layout.maximumHeight: menu.height

            width: settingsWidth
            y:0
        }

        DiplaySettingsViewer {
            id: appSettings
            Layout.alignment: Qt.AlignTop
            visible: menuDisplay.active
            Layout.maximumHeight: menu.height
            width: settingsWidth
            implicitWidth: settingsWidth

            y:0

            targetPlot: menu.targetPlot
        }

        Settings3DView {
            id: scene3DSettings
            Layout.alignment: Qt.AlignTop
            visible: menu3DSettings.active
            Layout.maximumHeight: menu.height
            width: settingsWidth
            implicitWidth: settingsWidth

            y:0
        }

    }
}
