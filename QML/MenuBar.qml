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

    signal languageChanged(string langStr)

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
        spacing: 0

        ColumnLayout {
            id: mainLayout
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: theme.controlHeight*1.2
            Layout.topMargin: 6
            spacing: 4
            Layout.margins: 4

            Component.onCompleted: {
                resetButtonOpacity()
            }

            function highlightAllButtons() {
                mainLayout.opacity = 1
            }

            function resetButtonOpacity() {
                mainLayout.opacity = 0.5
            }

            MenuButton {
                id: menuSettings
                icon.source: "./icons/plug.svg"
                Layout.fillWidth: true

                CMouseOpacityArea {
                    toolTipText: qsTr("Connections")
                    onContainsMouseChanged: containsMouse ? mainLayout.highlightAllButtons() : mainLayout.resetButtonOpacity()
                }

                onPressed: {
                    itemChangeActive(menuSettings)
                }
            }

            MenuButton {
                id: menuDisplay
                Layout.fillWidth: true
                icon.source: "./settings-outline.svg"

                CMouseOpacityArea {
                    toolTipText: qsTr("Settings")
                    onContainsMouseChanged: containsMouse ? mainLayout.highlightAllButtons() : mainLayout.resetButtonOpacity()
                }

                onPressed: {
                    itemChangeActive(menuDisplay)
                }
            }

            /*MenuButton {
                id: menu3DSettings
                visible: instruments > 0
                icon.source: "./3dcube.svg"

                Layout.fillWidth: true

                onPressed: {
                    itemChangeActive(menu3DSettings)
                }
            }*/

            CheckButton {
                id: settings3DButton
                visible: instruments > 0
                width: theme.controlHeight*1.2
                icon.source: "./icons/map.svg"
                backColor: theme.controlBackColor
                borderColor:  theme.controlBackColor
                checkedBorderColor: "black"

                CMouseOpacityArea {
                    toolTipText: qsTr("Display 3D")
                    onContainsMouseChanged: containsMouse ? mainLayout.highlightAllButtons() : mainLayout.resetButtonOpacity()
                }

                onClicked: {
                    if(!settings3DButton.checked && !visible2dButton.checked) {
                        visible2dButton.checked = true
                    }
                }
            }

            CheckButton {
                id: visible2dButton
                visible: instruments > 0
                width: theme.controlHeight*1.2
                icon.source: "./icons/ripple.svg"

                CMouseOpacityArea {
                    toolTipText: qsTr("Display 2D")
                    onContainsMouseChanged: containsMouse ? mainLayout.highlightAllButtons() : mainLayout.resetButtonOpacity()
                }

                backColor: theme.controlBackColor
                borderColor:  theme.controlBackColor
                checkedBorderColor: "black"

                checked: true
                onClicked: {
                    if(!settings3DButton.checked && !visible2dButton.checked) {
                        settings3DButton.checked = true
                    }
                }
            }

            //MouseOpacityArea {
            //    id: menuMouseArea
            //}
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

        /*SceneControlMenu {
            id:                 sceneControlMenu
            objectName:         "sceneControlMenu"
            Layout.alignment: Qt.AlignTop
            // Layout.topMargin:   10
            // Layout.alignment:   Qt.AlignLeft
            // Layout.fillWidth:   true
            width: settingsWidth
            implicitWidth: settingsWidth
            visible:            menu3DSettings.active
        }*/
    }

    function handleChildSignal(langStr) {
        languageChanged(langStr)
    }

    Component.onCompleted: {
        appSettings.languageChanged.connect(handleChildSignal)
    }
}
