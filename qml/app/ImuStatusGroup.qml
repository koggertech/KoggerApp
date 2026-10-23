import QtQuick 2.15
import kqml_types 1.0
import "StatusSeverity.js" as Sev

DeviceSettingsGroup {
    id: imuGroup

    property var dev: null

    title: qsTr("IMU")
    titlePixelSize: 13
    stateKey: "dev.imuStatus"
    collapsedByDefault: true
    visible: _valid

    readonly property bool _valid: !!(dev && dev.navSensorStatusValid)
    readonly property bool _stale: !!(dev && dev.navSensorStatusStale)
    readonly property int _state: dev ? dev.imuState : 0
    readonly property int _flags: dev ? dev.imuFaultFlags : 0
    readonly property int _samples: dev ? dev.imuSamplesPerS : 0
    readonly property int _fed: dev ? dev.imuEkfFedPerS : 0
    readonly property string sev: !_valid ? "idle" : _stale ? "stale" : _state === 0 ? "good" : "crit"

    function _hex(v) {
        var h = (v & 0xFF).toString(16).toUpperCase()
        return "0x" + (h.length < 2 ? "0" + h : h)
    }
    function _accG() { return dev ? (dev.imuAccNormMg / 1000).toFixed(3) : "0" }
    function _gyrDps() { return dev ? (dev.imuGyrNormCdps / 100).toFixed(2) : "0" }
    function _tempC() {
        if (!dev || dev.imuTempCc === -32768) return "—"
        return (dev.imuTempCc / 100).toFixed(1)
    }

    function _stateWord(st) {
        switch (st) {
        case 0: return qsTr("OK")
        case 1: return qsTr("Not found")
        case 2: return qsTr("Bus errors")
        case 3: return qsTr("No data")
        case 4: return qsTr("Frozen")
        case 5: return qsTr("Implausible")
        case 6: return qsTr("Restarting")
        }
        return qsTr("State %1").arg(st)
    }

    function _faultList(f) {
        var defs = [
            { b: 0x0001, n: qsTr("identity") },
            { b: 0x0002, n: qsTr("bus") },
            { b: 0x0004, n: qsTr("configuration lost") },
            { b: 0x0008, n: qsTr("low rate") },
            { b: 0x0010, n: qsTr("no-data reads") },
            { b: 0x0020, n: qsTr("frozen output") },
            { b: 0x0040, n: qsTr("acceleration") },
            { b: 0x0080, n: qsTr("temperature") },
            { b: 0x0100, n: qsTr("full scale") },
            { b: 0x8000, n: qsTr("test injection") }
        ]
        var out = []
        for (var i = 0; i < defs.length; ++i)
            if (f & defs[i].b)
                out.push(defs[i].n)
        return out.join(", ")
    }

    readonly property var hero: _hero()
    function _hero() {
        if (!_valid)
            return { word: qsTr("Waiting…"), sub: "" }
        if (_stale)
            return { word: qsTr("No response"), sub: qsTr("No IMU report for over 5 s — showing the last one.") }
        switch (_state) {
        case 0:
            return { word: qsTr("IMU OK"),
                     sub: _fed >= _samples ? qsTr("%1 samples/s, all used for navigation.").arg(_samples)
                                           : qsTr("%1 samples/s, %2 used for navigation.").arg(_samples).arg(_fed) }
        case 1:
            return { word: qsTr("IMU not found"),
                     sub: qsTr("WHO_AM_I reads %1: the IMU, its wiring or the main board does not answer.").arg(_hex(dev.imuWhoAmI)) }
        case 2:
            return { word: qsTr("IMU bus errors"),
                     sub: qsTr("%1 failed transfers in the last second.").arg(dev.imuBusErrCnt) }
        case 3:
            return { word: qsTr("No IMU data"),
                     sub: (_flags & 0x0004) ? qsTr("The IMU reset and lost its configuration.")
                                            : qsTr("%1 samples/s — too few for navigation.").arg(_samples) }
        case 4:
            return { word: qsTr("IMU output frozen"),
                     sub: qsTr("%1 identical samples in a row.").arg(dev.imuStuckRunMax) }
        case 5:
            return { word: qsTr("IMU readings implausible"),
                     sub: (_flags & 0x0040) ? qsTr("Mean acceleration %1 g.").arg(_accG())
                        : (_flags & 0x0080) ? qsTr("Temperature %1 °C.").arg(_tempC())
                        : "" }
        case 6:
            return { word: qsTr("IMU restarting"),
                     sub: qsTr("Re-initialised after repeated faults: %1.").arg(_faultList(_flags)) }
        }
        return { word: _stateWord(_state), sub: "" }
    }

    headerActions: Row {
        id: headerChip
        readonly property int gap: Tokens.spaceSm
        spacing: gap
        rightPadding: gap
        visible: imuGroup._valid
        Rectangle {
            id: chipFrame
            readonly property color accent: Sev.color(AppPalette, imuGroup.sev)
            implicitWidth: chipRow.implicitWidth + Tokens.spaceLg * 2
            width: implicitWidth
            height: imuGroup.headerActionSize - headerChip.gap * 2
            radius: height / 2
            anchors.verticalCenter: parent ? parent.verticalCenter : undefined
            color: "transparent"
            border.width: Math.max(1, Math.round(1.5 * AppPalette.scale))
            border.color: accent
            Row {
                id: chipRow
                anchors.centerIn: parent
                spacing: Math.round(6 * AppPalette.scale)
                Rectangle {
                    width: Math.round(8 * AppPalette.scale); height: width; radius: width / 2
                    anchors.verticalCenter: parent.verticalCenter
                    color: chipFrame.accent
                }
                Text {
                    text: imuGroup._stale ? qsTr("No response") : imuGroup._stateWord(imuGroup._state)
                    color: AppPalette.textStrong
                    font.pixelSize: Tokens.fontBase; font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }

    StatusBanner {
        width: parent.width
        sev: imuGroup.sev
        word: imuGroup.hero.word
        sub: imuGroup.hero.sub
        alert: imuGroup._valid && !imuGroup._stale && imuGroup._fed === 0
               ? qsTr("Navigation is not using the IMU.") : ""
        dimmed: imuGroup._stale

        Flow {
            width: parent.width
            spacing: Tokens.spaceSm

            StatusStatBox { blabel: "WHO_AM_I"; bvalue: imuGroup.dev ? imuGroup._hex(imuGroup.dev.imuWhoAmI) : "—"
                            alertBox: !!(imuGroup._flags & 0x0001) }
            StatusStatBox { blabel: qsTr("Samples/s"); bvalue: String(imuGroup._samples)
                            alertBox: !!(imuGroup._flags & 0x0008) }
            StatusStatBox { blabel: qsTr("Navigation/s"); bvalue: String(imuGroup._fed)
                            alertBox: imuGroup._valid && imuGroup._fed === 0 }
            StatusStatBox { blabel: "|a|"; bvalue: imuGroup._accG() + " g"
                            alertBox: !!(imuGroup._flags & 0x0040) }
            StatusStatBox { blabel: "|ω|"; bvalue: imuGroup._gyrDps() + " °/s" }
            StatusStatBox { blabel: qsTr("Temperature"); bvalue: imuGroup._tempC() + " °C"
                            alertBox: !!(imuGroup._flags & 0x0080) }
            StatusStatBox { blabel: qsTr("Restarts"); bvalue: imuGroup.dev ? String(imuGroup.dev.imuReinitCnt) : "0" }
        }

        Text {
            width: parent.width
            visible: imuGroup._flags !== 0
            text: qsTr("Faults: %1").arg(imuGroup._faultList(imuGroup._flags))
            color: AppPalette.textSecond; font.pixelSize: Tokens.fontXs
            wrapMode: Text.WordWrap
        }

        Text {
            width: parent.width
            text: imuGroup.dev ? qsTr("Bus errors %1 · no-data reads %2 · longest identical run %3 · report #%4")
                            .arg(imuGroup.dev.imuBusErrCnt).arg(imuGroup.dev.imuInvalidCnt)
                            .arg(imuGroup.dev.imuStuckRunMax).arg(imuGroup.dev.navSensorSeq)
                      : ""
            color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
            wrapMode: Text.WordWrap
        }
    }
}
