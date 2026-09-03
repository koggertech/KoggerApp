import QtQuick 2.15
import kqml_types 1.0

Column {
    id: page

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

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
    }
}
