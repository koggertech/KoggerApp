import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtCore


Item {
    id: menu
    implicitWidth: menuLayout.width

    property var targetPlot: null

    property var lastItem: menuSettings
    //property bool isConsoleVisible: consoleEnable.checked // TODO
    property bool is3DVisible: visible3DButton.checked
    property bool is2DVisible: visible2DButton.checked
    property int numPlots: appSettings.numPlots
    property bool syncPlots: appSettings.syncPlots
    property int instruments:  appSettings.instruments
    property int settingsWidth: theme.controlHeight*20
    property string filePath: devSettings.filePath
    property bool extraInfoVis: appSettings.extraInfoVis
    property bool extraInfoDepthVis: appSettings.extraInfoDepthVis
    property bool extraInfoSpeedVis: appSettings.extraInfoSpeedVis
    property bool extraInfoCoordinatesVis: appSettings.extraInfoCoordinatesVis
    property bool extraInfoActivePointVis: appSettings.extraInfoActivePointVis
    property bool extraInfoSimpleNavV2Vis: appSettings.extraInfoSimpleNavV2Vis
    property bool extraInfoBoatStatusVis: appSettings.extraInfoBoatStatusVis
    property bool autopilotInfofVis: appSettings.autopilotInfofVis
    property bool profilesBtnVis: appSettings.profilesButtonVis

    signal languageChanged(string langStr)
    signal menuBarSettingOpened()
    signal syncPlotEnabled()

    function updateBottomTrack() {
        appSettings.updateBottomTrack()
    }
    function applyProfileToAllDevices(path) {
        if (!path || !path.length) {
            return
        }
        if (devSettings && devSettings.importProfileForAllDevices) {
            devSettings.importProfileForAllDevices(path)
        }
    }

    function clickConnections() {
        itemChangeActive(menuSettings)
    }

    function clickSettings() {
        itemChangeActive(menuDisplay)
    }

    function click2D() {
        visible2DButton.checked = !visible2DButton.checked;
        visible2DButton.clicked()
    }

    function click3D() {
        visible3DButton.checked = !visible3DButton.checked;
        visible3DButton.clicked()
    }

    function closeMenus() {
        if (menuSettings.active) {
            menuSettings.active = false
        }
        if (menuDisplay.active) {
            menuDisplay.active = false
        }
    }

    function itemChangeActive(currentItem) {
        let wasOpen = currentItem.active
        let lastItemTmp = lastItem

        if (currentItem) {
            currentItem.active = !(currentItem.active)
        }

        if (lastItem && lastItem !== currentItem) {
            lastItem.active = false
        }

        lastItem = currentItem

        if (!wasOpen && currentItem.active && (currentItem === menuSettings || currentItem === menuDisplay)) {
            menuBarSettingOpened()
        }
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
                icon.source: "qrc:/icons/ui/plug.svg"
                Layout.fillWidth: true
                isKlfLogging: core.isKlfLogging
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

            CheckButton {
                id: visible3DButton
                implicitWidth: theme.controlHeight * 1.2
                icon.source: "qrc:/icons/ui/map.svg"
                backColor: theme.controlBackColor
                borderColor:  theme.controlBackColor
                checkedBorderColor: "black"
                checked: true

                CMouseOpacityArea {
                    toolTipText: qsTr("Display 3D")
                    onContainsMouseChanged: containsMouse ? mainLayout.highlightAllButtons() : mainLayout.resetButtonOpacity()
                }

                onClicked: {
                    if (!visible3DButton.checked && !visible2DButton.checked) {
                        visible2DButton.checked = true
                    }
                }

                Settings {
                    id: visible3DSettings
                    property alias visible3DButtonChecked: visible3DButton.checked
                }
            }

            CheckButton {
                id: visible2DButton
                implicitWidth: theme.controlHeight * 1.2
                icon.source: "qrc:/icons/ui/ripple.svg"
                backColor: theme.controlBackColor
                borderColor:  theme.controlBackColor
                checkedBorderColor: "black"
                checked: true

                CMouseOpacityArea {
                    toolTipText: qsTr("Display 2D")
                    onContainsMouseChanged: containsMouse ? mainLayout.highlightAllButtons() : mainLayout.resetButtonOpacity()
                }

                onClicked: {
                    if (!visible3DButton.checked && !visible2DButton.checked) {
                        visible3DButton.checked = true
                    }
                }

                Settings {
                    id: visible2DSettings
                    property alias visible2DButtonChecked: visible2DButton.checked
                }
            }
        }

        DeviceSettingsViewer {
            id: devSettings
            visible: menuSettings.active
            Layout.maximumHeight: menu.height
            menuWidth: settingsWidth
            y:0
        }

        DisplaySettingsViewer {
            id: appSettings
            Layout.alignment: Qt.AlignTop
            visible: menuDisplay.active
            Layout.maximumHeight: menu.height
            menuWidth: settingsWidth

            y:0

            targetPlot: menu.targetPlot
        }
    }

    function handleChildSignal(langStr) {
        languageChanged(langStr)
    }

    function handleSyncPlotEnabled() {
        syncPlotEnabled()
    }

    Component.onCompleted: {
        appSettings.languageChanged.connect(handleChildSignal)
        appSettings.syncPlotEnabled.connect(handleSyncPlotEnabled)
    }
}
