import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import Qt.labs.settings 1.1

Item {
    id: menu
    implicitWidth: menuLayout.width

    property var targetPlot: null

    property var lastItem: menuSettings
    //property bool isConsoleVisible: consoleEnable.checked // TODO
    property bool is3DVisible: settings3DButton.checked
    property bool is2DVisible: visible2dButton.checked
    property bool is2DHorizontal: appSettings.is2DHorizontal
    property int instruments:  appSettings.instruments
    property int settingsWidth: theme.controlHeight*20

    function itemChangeActive(currentItem) {
        if(currentItem) {
            currentItem.active = !(currentItem.active)
        }

        if(lastItem && lastItem !== currentItem) {
            lastItem.active = false
        }

        lastItem = currentItem
    }

    RowLayout {
        id: menuLayout
        Layout.fillWidth: true

        ColumnLayout {
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: theme.controlHeight*1.5
            Layout.topMargin: 6
            spacing: 4
            Layout.margins: 4

            MenuButton {
                id: menuSettings
                icon.source: "./code-working.svg"
                Layout.fillWidth: true

                onPressed: {
                    itemChangeActive(menuSettings)
                }
            }

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

            CButton {
                id: settings3DButton
                visible: instruments > 0
                Layout.fillWidth: true
                text: "3D"
                checkable: true

                onClicked: {
//                        Settings3DController.changeSceneVisibility(settings3DButton.checked)
//                       itemChangeActive(settings3DButton)
                }
            }

            CButton {
                id: visible2dButton
                visible: instruments > 0
                Layout.fillWidth: true
                text: "2D"
                checkable: true
                checked: true
                onClicked: {
//                        if(checked) { core.movePoints() }
                }
            }


            // ColumnLayout {
            //     Layout.alignment: Qt.AlignHCenter
            //     visible: visible2dButton.checked

            //     MenuBlock {
            //     }


            //     CText {
            //         Layout.fillWidth: true
            //         Layout.topMargin: 4
            //         // visible: chartEnable.checked // TODO
            //         horizontalAlignment: Text.AlignHCenter
            //         text: echogramLevelsSlider.stopValue
            //         small: true
            //     }

            //     ChartLevel {
            //         Layout.fillWidth: true
            //         id: echogramLevelsSlider
            //         // visible: chartEnable.checked // TODO
            //         Layout.alignment: Qt.AlignHCenter

            //         onStartValueChanged: {
            //             targetPlot.plotEchogramSetLevels(startValue, stopValue);
            //         }

            //         onStopValueChanged: {
            //             targetPlot.plotEchogramSetLevels(startValue, stopValue);
            //         }

            //         Component.onCompleted: {
            //             targetPlot.plotEchogramSetLevels(startValue, stopValue);
            //         }

            //         Settings {
            //             property alias echogramLevelsStart: echogramLevelsSlider.startValue
            //             property alias echogramLevelsStop: echogramLevelsSlider.stopValue
            //         }
            //     }

            //     CText {
            //         Layout.fillWidth: true
            //         Layout.bottomMargin: 4
            //         // visible: chartEnable.checked // TODO
            //         horizontalAlignment: Text.AlignHCenter

            //         text: echogramLevelsSlider.startValue
            //         small: true
            //     }
            // }


        }



        DeviceSettingsViewer {
            id: devSettings
            visible: menuSettings.active
            Layout.maximumHeight: menu.height
            menuWidth: settingsWidth
            y:0
        }



        DiplaySettingsViewer {
            id: appSettings
            Layout.alignment: Qt.AlignTop
            visible: menuDisplay.active
            Layout.maximumHeight: menu.height
            menuWidth: settingsWidth

            y:0

            targetPlot: menu.targetPlot
        }

        SceneControlMenu {
            id:                 sceneControlMenu
            objectName:         "sceneControlMenu"
            Layout.alignment: Qt.AlignTop
            // Layout.topMargin:   10
            // Layout.alignment:   Qt.AlignLeft
            // Layout.fillWidth:   true
            width: settingsWidth
            implicitWidth: settingsWidth
            visible:            menu3DSettings.active
        }
    }
}
