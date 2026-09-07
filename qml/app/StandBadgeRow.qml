import QtQuick 2.15
import kqml_types 1.0
import "StandLogic.js" as Stand

// The scan, stated as marks. This file owns every word of that vocabulary — StandLogic returns
// codes and numbers — so the panel and anything else that reports a scan cannot describe one
// configuration in two ways.
//
// A mark is struck through when it differs from what was last sent. With no banner and no
// readback, that strike is the only place the panel says the form and the device disagree.
Flow {
    id: row

    property var config: null
    property var sentConfig: null
    property real transparencyAlpha: 1.0

    readonly property var _marks: Stand.marks(config, sentConfig)

    spacing: Math.round(4 * AppPalette.scale)

    function _text(m) {
        switch (m.key) {
        case "order":     return _axis(m.inner) + " → " + _axis(m.outer)
        case "grid":      return m.inner + " × " + m.outer + qsTr(" pts")
        case "roundTrip": return qsTr("round-trip")
        case "reset":     return qsTr("reset")
        case "sweep":     return qsTr("sweep")
        case "fires":     return m.n + qsTr(" fire/pt")
        case "noFire":    return qsTr("no fire")
        case "cycles":    return "× " + m.n + qsTr(" cycles")
        case "timing":    return m.postFire >= 0 ? (m.settle + " / " + m.postFire + qsTr(" ms"))
                                                 : (m.settle + qsTr(" ms"))
        }
        return ""
    }

    function _axis(a) { return a === "el" ? qsTr("el") : qsTr("az") }

    Repeater {
        model: row._marks

        Rectangle {
            required property var modelData

            readonly property bool _mode:  modelData.tone === "mode"
            readonly property bool _sweep: modelData.tone === "sweep"
            readonly property bool _warn:  modelData.tone === "warn"

            height: Math.round(Tokens.chipH * 0.82)
            width: label.implicitWidth + Math.round(16 * AppPalette.scale)
            radius: height / 2

            color: _sweep || _warn ? Qt.rgba(AppPalette.linkIdleBg.r, AppPalette.linkIdleBg.g,
                                             AppPalette.linkIdleBg.b, row.transparencyAlpha)
                 : _mode           ? Qt.rgba(AppPalette.accentBg.r, AppPalette.accentBg.g,
                                             AppPalette.accentBg.b, 0.18 * row.transparencyAlpha)
                                   : Qt.rgba(AppPalette.rowRaised.r, AppPalette.rowRaised.g,
                                             AppPalette.rowRaised.b, row.transparencyAlpha)
            border.width: 1
            border.color: _sweep || _warn ? AppPalette.linkIdleBorder
                        : _mode           ? AppPalette.accentBorder
                                          : AppPalette.border

            Text {
                id: label
                anchors.centerIn: parent
                text: row._text(parent.modelData)
                font.pixelSize: Tokens.fontXxs
                font.strikeout: !!parent.modelData.stale
                color: parent._sweep || parent._warn ? AppPalette.linkIdleText
                     : parent._mode                  ? AppPalette.accent
                                                     : AppPalette.textSecond
            }
        }
    }
}
