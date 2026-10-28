import QtQuick 2.15
import kqml_types 1.0

Column {
    id: page

    required property var store

    readonly property bool hasCore: typeof core !== "undefined" && core
    property string appLogPath: hasCore ? core.appLogFilePath() : ""

    Connections {
        target: page.hasCore ? core : null
        function onAppLogPathChanged() { page.appLogPath = core.appLogFilePath() }
    }

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

    component LogButton: KButton {
        width: Math.round(124 * AppPalette.scale)
        height: Tokens.controlHMd
        fontPixelSize: Tokens.fontMd
        normalBg: AppPalette.controlRaised
        hoverBg: Qt.lighter(AppPalette.controlRaised, 1.2)
    }

    KIsland {
        KIslandRow {
            label: qsTr("Developer mode")
            toolTipText: qsTr("Unlocks panels and controls meant for development. Turning it off hides this page.")
            interactive: true
            onClicked: developerModeSwitch.click()

            KSwitch {
                id: developerModeSwitch
                flat: true
                checked: page.store ? page.store.developerMode : false
                onToggled: {
                    if (page.store)
                        page.store.developerMode = checked
                    checked = Qt.binding(function() { return page.store ? page.store.developerMode : false })
                }
            }
        }
    }

    KIsland {
        KIslandRow {
            label: qsTr("Isobaths pipeline status")
            toolTipText: qsTr("Shows the bathymetry pipeline state in the Isobaths group. Polling adds load to the pipeline — keep it off unless diagnosing.")
            interactive: true
            onClicked: isobathsStatusSwitch.click()

            KSwitch {
                id: isobathsStatusSwitch
                flat: true
                checked: page.store ? page.store.isobathsStatusMonitor : true
                onToggled: {
                    if (page.store)
                        page.store.isobathsStatusMonitor = checked
                    checked = Qt.binding(function() { return page.store ? page.store.isobathsStatusMonitor : true })
                }
            }
        }

        KIslandRow {
            label: qsTr("Mosaic pipeline status")
            toolTipText: qsTr("Shows the mosaic pipeline state in the Mosaic group. Polling adds load to the pipeline — keep it off unless diagnosing.")
            interactive: true
            onClicked: mosaicStatusSwitch.click()

            KSwitch {
                id: mosaicStatusSwitch
                flat: true
                checked: page.store ? page.store.mosaicStatusMonitor : true
                onToggled: {
                    if (page.store)
                        page.store.mosaicStatusMonitor = checked
                    checked = Qt.binding(function() { return page.store ? page.store.mosaicStatusMonitor : true })
                }
            }
        }
    }

    KIsland {
        KIslandRow {
            label: qsTr("Application log")
            caption: page.appLogPath.length ? page.appLogPath : qsTr("Log file is not active")
            stacked: true
            verticalPadding: Tokens.spaceMd

            Row {
                spacing: Tokens.spaceSm

                LogButton {
                    text: qsTr("Open folder")
                    visible: Qt.platform.os !== "android"
                    enabled: page.appLogPath.length > 0
                    onClicked: core.revealAppLogFolder()
                }

                LogButton {
                    text: qsTr("Copy path")
                    enabled: page.appLogPath.length > 0
                    onClicked: core.copyToClipboard(page.appLogPath)
                }
            }
        }
    }
}
