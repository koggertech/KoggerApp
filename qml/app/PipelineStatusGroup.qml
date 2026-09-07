import QtQuick 2.15
import kqml_types 1.0

SettingsGroup {
    id: panel

    property bool active: false
    property var ctrl: null
    property var sections: []
    property var fmt: null
    property var verdict: ({ text: "", ok: false, neutral: true })

    visible: active
    width: parent ? parent.width : preferredWidth
    preferredWidth: width

    title: qsTr("Pipeline status")
    contentSpacing: 0
    collapsedByDefault: true
    scrollIntoViewOnExpand: false
    headerTint: verdict.neutral ? "transparent"
                : verdict.ok ? AppPalette.linkOkBg
                : AppPalette.dangerBg

    onActiveChanged: _applyMonitor()
    onExpandedChanged: _applyMonitor()
    Component.onCompleted: _applyMonitor()
    Component.onDestruction: if (ctrl) ctrl.statusMonitorEnabled = false

    function _applyMonitor() {
        if (!ctrl)
            return

        ctrl.statusDetailedPolling = panel.expanded
        ctrl.statusMonitorEnabled = panel.active
    }

    component StatusSection: Column {
        id: section

        required property string title
        required property var rows
        required property var fmt
        property color flashColor: AppPalette.linkOkBorder

        spacing: 0

        Item {
            width: section.width
            height: Math.round(Tokens.fontSm * 1.5) + Tokens.spaceXs

            Text {
                text: section.title
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontSm
                font.bold: true
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 0.4
                anchors.left: parent.left
                anchors.top: parent.top
            }

            Rectangle {
                height: Math.max(1, Math.round(AppPalette.scale))
                color: AppPalette.separator
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.bottomMargin: Math.round(Tokens.spaceXs / 2)
            }
        }

        Repeater {
            model: section.rows

            Item {
                id: statusRow

                width: section.width
                height: Math.round(Tokens.fontBase * 1.6)

                Text {
                    text: modelData.label
                    color: AppPalette.textMuted
                    font.pixelSize: Tokens.fontSm
                    elide: Text.ElideRight
                    anchors.left: parent.left
                    anchors.right: rowValue.left
                    anchors.rightMargin: Tokens.spaceSm
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    id: rowValue

                    property real highlight: 0.0

                    readonly property color baseColor: text === "—" ? AppPalette.textMuted : AppPalette.text

                    text: section.fmt ? section.fmt(modelData) : ""
                    color: highlight > 0.0
                           ? Qt.tint(baseColor, Qt.rgba(section.flashColor.r,
                                                        section.flashColor.g,
                                                        section.flashColor.b,
                                                        highlight))
                           : baseColor
                    font.pixelSize: Tokens.fontSm
                    font.bold: true
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter

                    onTextChanged: highlightAnim.restart()

                    SequentialAnimation {
                        id: highlightAnim

                        NumberAnimation {
                            target: rowValue
                            property: "highlight"
                            to: 1.0
                            duration: 150
                            easing.type: Easing.OutQuad
                        }
                        NumberAnimation {
                            target: rowValue
                            property: "highlight"
                            to: 0.0
                            duration: 1200
                            easing.type: Easing.InOutQuad
                        }
                    }
                }
            }
        }
    }

    Item {
        id: verdictSlot

        readonly property int sideInset: Tokens.spaceMd
        readonly property int innerPad: Tokens.spaceMd + Tokens.spaceXs
        readonly property int topGap: Tokens.spaceMd
        readonly property int bottomGap: Tokens.spaceMd + Tokens.spaceSm

        width: parent.width
        height: verdictBox.height + topGap + bottomGap

        Rectangle {
            id: verdictBox

            readonly property color accent: panel.verdict.neutral ? AppPalette.border
                                            : panel.verdict.ok ? AppPalette.linkOkBorder
                                            : AppPalette.dangerBorder
            readonly property int dotSize: Math.round(6 * AppPalette.scale)

            anchors.top: parent.top
            anchors.topMargin: verdictSlot.topGap
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: verdictSlot.sideInset
            anchors.rightMargin: verdictSlot.sideInset

            radius: Tokens.radiusMd
            height: verdictText.implicitHeight + verdictSlot.innerPad * 2
            color: panel.verdict.neutral ? AppPalette.bg
                   : panel.verdict.ok ? AppPalette.linkOkBg
                   : AppPalette.dangerBg
            border.width: Math.max(1, Math.round(AppPalette.scale))
            border.color: verdictBox.accent

            Behavior on color { ColorAnimation { duration: Anim.disclosureMs } }
            Behavior on border.color { ColorAnimation { duration: Anim.disclosureMs } }

            Rectangle {
                width: verdictBox.dotSize
                height: width
                radius: width / 2
                color: verdictBox.accent
                anchors.left: parent.left
                anchors.leftMargin: verdictSlot.innerPad
                anchors.verticalCenter: parent.verticalCenter

                Behavior on color { ColorAnimation { duration: Anim.disclosureMs } }
            }

            Text {
                id: verdictText

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: verdictSlot.innerPad * 2 + verdictBox.dotSize
                anchors.rightMargin: verdictSlot.innerPad
                text: panel.verdict.text
                wrapMode: Text.WordWrap
                font.pixelSize: Tokens.fontBase
                color: panel.verdict.neutral ? AppPalette.textSecond
                       : panel.verdict.ok ? AppPalette.linkOkText
                       : AppPalette.dangerText

                Behavior on color { ColorAnimation { duration: Anim.disclosureMs } }
            }
        }
    }

    Grid {
        id: grid

        readonly property int minColW: Math.round(150 * AppPalette.scale)

        width: parent.width
        columnSpacing: Tokens.spaceLg
        rowSpacing: Tokens.spaceLg
        columns: width <= 0 ? 3
                 : (width - columnSpacing * 2) / 3 >= minColW ? 3
                 : (width - columnSpacing) / 2 >= minColW ? 2
                 : 1

        readonly property real colW: Math.floor((width - columnSpacing * (columns - 1)) / columns)

        Repeater {
            model: panel.sections

            Item {
                id: sectionSlot

                required property var modelData

                width: grid.colW
                implicitHeight: sectionBody.implicitHeight
                height: implicitHeight

                StatusSection {
                    id: sectionBody

                    width: sectionSlot.width
                    title: sectionSlot.modelData ? sectionSlot.modelData.title : ""
                    rows: sectionSlot.modelData ? sectionSlot.modelData.rows : []
                    fmt: panel.fmt
                }
            }
        }
    }
}
