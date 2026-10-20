import QtQuick
import QtQuick.Layouts
import kqml_types 1.0

Column {
    id: page

    required property var store

    readonly property var controller: (typeof tutorial !== "undefined" && tutorial) ? tutorial : null
    readonly property bool developerMode: !!(page.store && page.store.developerMode)
    readonly property var visibleScenarios: page.controller
                                            ? page.controller.scenarios.filter(function (s) { return !s.devOnly || page.developerMode })
                                            : []
    readonly property bool canStart: !!page.controller && !page.controller.active
                                     && !(page.store && page.store.inputLocked)

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

    function canResume(s) {
        return !s.completed && s.resumeStep > 0
    }

    function launch(s, resume) {
        if (!page.canStart)
            return
        if (resume)
            page.controller.resume(s.id)
        else
            page.controller.start(s.id)
    }

    function statusText(s) {
        if (s.completed)
            return qsTr("Completed")
        if (page.canResume(s))
            return qsTr("Stopped at step %1 of %2").arg(s.resumeStep + 1).arg(s.stepCount)
        return qsTr("%n step(s)", "", s.stepCount)
    }

    KIsland {
        title: qsTr("Tutorials")
        footer: page.visibleScenarios.length === 0 ? qsTr("No tutorials available yet") : ""

        Repeater {
            model: page.visibleScenarios

            KIslandRow {
                id: scenarioRow

                required property var modelData
                readonly property bool resumable: page.canResume(modelData)

                label: modelData.title
                caption: page.statusText(modelData)
                interactive: page.canStart
                onClicked: page.launch(modelData, scenarioRow.resumable)

                RowLayout {
                    spacing: Tokens.spaceSm

                    KButton {
                        visible: scenarioRow.modelData.completed || scenarioRow.resumable
                        text: qsTr("Reset")
                        bold: false
                        Layout.preferredHeight: Tokens.controlHMd
                        onClicked: page.controller.resetProgress(scenarioRow.modelData.id)
                    }

                    KButton {
                        visible: scenarioRow.resumable
                        text: qsTr("Restart")
                        enabled: page.canStart
                        Layout.preferredHeight: Tokens.controlHMd
                        onClicked: page.launch(scenarioRow.modelData, false)
                    }

                    KButton {
                        text: scenarioRow.resumable ? qsTr("Continue") : qsTr("Start")
                        enabled: page.canStart
                        normalBg: AppPalette.accentBg
                        hoverBg: AppPalette.accentBgHover
                        normalBorder: AppPalette.accentBorder
                        hoverBorder: AppPalette.accentBorder
                        textColor: AppPalette.accentText
                        Layout.preferredHeight: Tokens.controlHMd
                        onClicked: page.launch(scenarioRow.modelData, scenarioRow.resumable)
                    }
                }
            }
        }
    }

    KIsland {
        visible: page.visibleScenarios.length > 0

        KIslandRow {
            label: qsTr("Reset all tutorial progress")
            labelColor: AppPalette.dangerText
            interactive: true
            onClicked: if (page.controller) page.controller.resetProgress("")
        }
    }
}
