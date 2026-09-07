import QtQuick 2.15
import kqml_types 1.0

Column {
    id: body

    property real baseW: Math.round(300 * AppPalette.scale)
    property real ctlW: Math.round(118 * AppPalette.scale)

    readonly property bool _manualTesting: (typeof manualTesting !== "undefined") && manualTesting === true

    property QtObject _mockDev: QtObject {
        property bool isMock: true
        property bool isBoardInited: true
        property bool isServoSupport: true
        property string devName: "Recorder"
        property int devSN: 90001
        property bool servoEnabled: false
        property bool servoReverse: false
        property int servoPwmMinUs: 500
        property int servoPwmMaxUs: 2500
        property real servoAngleRangeDeg: 180
        property real servoStepDeg: 4.5
        property real servoRangeDeg: 180
        property real servoCenterDeg: 0
        property real servoCurrentAngleDeg: 0
        property int pwmRouteOut1: 1
        property int pwmRouteOut2: 0
        property int pwmRouteOut3: 0
    }

    property Timer _mockSweep: Timer {
        interval: 200
        repeat: true
        running: body.active && body._manualTesting && body.dev === body._mockDev && body._mockDev.servoEnabled
        property int dir: 1
        onTriggered: {
            var m = body._mockDev
            var half = m.servoRangeDeg / 2
            var next = m.servoCurrentAngleDeg + dir * Math.max(0.1, Math.abs(m.servoStepDeg))
            if (next > m.servoCenterDeg + half) { next = m.servoCenterDeg + half; dir = -1 }
            else if (next < m.servoCenterDeg - half) { next = m.servoCenterDeg - half; dir = 1 }
            m.servoCurrentAngleDeg = next
        }
    }

    readonly property var _allDevs: (typeof deviceManagerWrapper !== "undefined" && deviceManagerWrapper)
                                    ? deviceManagerWrapper.devs : []
    readonly property var _devs: {
        var out = []
        for (var i = 0; i < _allDevs.length; ++i) {
            var d = _allDevs[i]
            if (d && d.isBoardInited && d.isServoSupport)
                out.push(d)
        }
        if (out.length === 0 && _manualTesting)
            out.push(_mockDev)
        return out
    }

    property int _selSN: -1
    readonly property int _selIndex: {
        for (var i = 0; i < _devs.length; ++i)
            if (_devs[i] && _devs[i].devSN === _selSN)
                return i
        return 0
    }
    readonly property var dev: _devs.length > 0 ? _devs[_selIndex] : null

    readonly property var _devNames: {
        var out = []
        for (var i = 0; i < _devs.length; ++i)
            out.push(_devLabel(_devs[i]))
        return out
    }

    readonly property int _maxTabs: 3
    readonly property bool _tabbedPicker: _devs.length > 1 && _devs.length <= _maxTabs
    readonly property var _devTabs: {
        var out = []
        for (var i = 0; i < _devs.length; ++i) {
            var d = _devs[i]
            if (!d)
                continue
            var n = (d.devName && d.devName.length > 0) ? d.devName : ("#" + d.devSN)
            out.push({ label: n, value: d.devSN })
        }
        return out
    }

    readonly property var _pwmTargets: [qsTr("Off"), qsTr("ServoScan")]

    property bool _advanced: false

    function _devLabel(d) {
        if (!d)
            return ""
        var n = (d.devName && d.devName.length > 0) ? d.devName : ("@" + d.devAddress)
        return d.devSN > 0 ? n + " · " + d.devSN : n
    }

    readonly property real _sentinelDeg: -245.76

    readonly property var _ds: (typeof dataset !== "undefined") ? dataset : null
    readonly property real _axYaw: _ds ? _ds.lastYaw : NaN
    readonly property real _axPitch: _ds ? _ds.lastPitch : NaN
    readonly property real _axRoll: _ds ? _ds.lastRoll : NaN

    function _axValid(v) {
        return v !== undefined && v !== null && !isNaN(v) && Math.abs(v - _sentinelDeg) > 0.005
    }

    readonly property real _angleDeg: {
        if (dev && dev.isMock === true)
            return dev.servoCurrentAngleDeg
        var valid = []
        if (_axValid(_axYaw))   valid.push(_axYaw)
        if (_axValid(_axPitch)) valid.push(_axPitch)
        if (_axValid(_axRoll))  valid.push(_axRoll)
        if (valid.length === 1)
            return valid[0]
        if (_axValid(_axRoll))
            return _axRoll
        return NaN
    }

    function _linkLabelFor(d) {
        if (!d)
            return ""
        if (d.isMock === true)
            return "COM7 · 115200"
        if (typeof deviceTopology === "undefined" || !deviceTopology)
            return ""
        var groups = deviceTopology.groups
        if (!groups)
            return ""
        var g = deviceTopology.groupForDevice(d)
        if (!g)
            return ""
        if (g.portName && g.portName.length > 0)
            return g.baudrate > 0 ? g.portName + " · " + g.baudrate : g.portName
        if (g.address && g.address.length > 0)
            return g.destinationPort > 0 ? g.address + ":" + g.destinationPort : g.address
        return ""
    }

    readonly property string _linkLabel: _linkLabelFor(dev)

    property bool active: true

    function _log(msg) {
        if (active && typeof core !== "undefined" && core && core.consoleInfo)
            core.consoleInfo("[Servo] " + msg)
    }

    property int _attCount: 0
    property string _lastAttLine: ""

    function _fmt(v) { return (v === undefined || v === null || isNaN(v)) ? "—" : v.toFixed(2) }

    Connections {
        target: body._ds
        ignoreUnknownSignals: true

        function onAttitudeUpdated() { body._attCount++ }
    }

    Connections {
        target: body.dev
        ignoreUnknownSignals: true

        function onServoControlChanged() {
            var d = body.dev
            if (!d)
                return
            body._log("device: enabled=" + d.servoEnabled + " reverse=" + d.servoReverse
                      + "° step=" + d.servoStepDeg.toFixed(2) + "° range=" + d.servoRangeDeg.toFixed(0)
                      + "° center=" + d.servoCenterDeg.toFixed(0)
                      + "° angleRange=" + d.servoAngleRangeDeg.toFixed(0)
                      + "° pwm=" + d.servoPwmMinUs + ".." + d.servoPwmMaxUs + "µs")
        }

        function onPwmRouteChanged() {
            var d = body.dev
            if (!d)
                return
            body._log("device: pwm route OUT1=" + d.pwmRouteOut1
                      + " OUT2=" + d.pwmRouteOut2 + " OUT3=" + d.pwmRouteOut3)
        }
    }

    property Timer _attReport: Timer {
        interval: 2000
        repeat: true
        running: body.active && !!body.dev
        onTriggered: {
            var line = "attitude: " + body._attCount + " updates / 2 s, y/p/r = "
                       + body._fmt(body._axYaw) + " / " + body._fmt(body._axPitch)
                       + " / " + body._fmt(body._axRoll) + "°"
            body._attCount = 0
            if (line !== body._lastAttLine) {
                body._lastAttLine = line
                body._log(line)
            }
        }
    }

    component DevSpin: Item {
        id: sp
        property int devValue: 0
        property int from: 0
        property int to: 100
        property int stepSize: 1
        property real divisor: 1.0
        property int decimals: 0
        property var writeBack: null
        property bool _in: false

        implicitWidth: body.ctlW
        implicitHeight: Tokens.controlHMd

        onDevValueChanged: if (spin.value !== devValue) { _in = true; spin.value = devValue; _in = false }
        Component.onCompleted: { _in = true; spin.value = devValue; _in = false }

        KSpinBox {
            id: spin
            anchors.fill: parent
            from: sp.from; to: sp.to; stepSize: sp.stepSize
            divisor: sp.divisor; decimals: sp.decimals
            onValueModified: function(v) { if (!sp._in && sp.writeBack) sp.writeBack(v) }
        }
    }

    component DevSwitch: KSwitch {
        property bool wantChecked: false
        property var writeBack: null
        property bool _g: false

        flat: true
        onWantCheckedChanged: if (checked !== wantChecked) { _g = true; checked = wantChecked; _g = false }
        Component.onCompleted: { _g = true; checked = wantChecked; _g = false }
        onToggled: if (!_g && writeBack) writeBack(checked)
    }

    component DevCombo: KCombo {
        property int wantIndex: 0
        property var writeBack: null
        property bool _g: false

        width: body.ctlW
        height: Tokens.controlHMd
        fontPixelSize: Tokens.fontBase
        onWantIndexChanged: if (currentIndex !== wantIndex) { _g = true; currentIndex = wantIndex; _g = false }
        Component.onCompleted: { _g = true; currentIndex = wantIndex; _g = false }
        onActivated: function(i) { if (!_g && writeBack) writeBack(i) }
    }

    width: baseW
    spacing: Tokens.spaceMd

    KIsland {
        KIslandRow {
            label: body.dev ? body._devLabel(body.dev) : qsTr("No servo device")
            labelColor: body.dev ? AppPalette.textStrong : AppPalette.textMuted
            caption: (body.dev && body._linkLabel.length > 0) ? qsTr("On link %1").arg(body._linkLabel) : ""
        }

        KIslandRow {
            visible: body._tabbedPicker
            stacked: true

            KTabBar {
                width: parent.width
                options: body._devTabs
                currentValue: body.dev ? body.dev.devSN : -1
                trackColor: AppPalette.bgDeep
                fontPixelSize: Tokens.fontBase
                onValueSelected: function(v) { body._selSN = v }
            }
        }

        KIslandRow {
            visible: body._devs.length > body._maxTabs
            label: qsTr("Device")

            DevCombo {
                model: body._devNames
                wantIndex: body._selIndex
                writeBack: function(i) {
                    if (i >= 0 && i < body._devs.length && body._devs[i])
                        body._selSN = body._devs[i].devSN
                }
            }
        }
    }

    KIsland {
        visible: !!body.dev
        title: qsTr("Control")

        KIslandRow {
            label: qsTr("Enabled")

            DevSwitch {
                wantChecked: !!(body.dev && body.dev.servoEnabled)
                writeBack: function(v) {
                    if (body.dev && body.dev.servoEnabled !== v) {
                        body._log("write enabled = " + v)
                        body.dev.servoEnabled = v
                    }
                }
            }
        }

        KIslandRow {
            label: qsTr("Current angle")

            Text {
                text: body._axValid(body._angleDeg) ? body._angleDeg.toFixed(2) + "°" : "—"
                color: AppPalette.textStrong
                font.pixelSize: Tokens.fontLg
                font.bold: true
            }
        }

        KIslandRow {
            label: qsTr("Reverse mapping")

            DevSwitch {
                wantChecked: !!(body.dev && body.dev.servoReverse)
                writeBack: function(v) {
                    if (body.dev && body.dev.servoReverse !== v) {
                        body._log("write reverse = " + v)
                        body.dev.servoReverse = v
                    }
                }
            }
        }
    }

    KIsland {
        visible: !!body.dev
        title: qsTr("Scan")

        KIslandRow {
            label: qsTr("Step, °")

            DevSpin {
                from: -3600; to: 3600; stepSize: 1; divisor: 10; decimals: 1
                devValue: body.dev ? Math.round(body.dev.servoStepDeg * 10) : 0
                writeBack: function(v) {
                    if (!body.dev)
                        return
                    var deg = v / 10.0
                    if (Math.abs(body.dev.servoStepDeg - deg) > 0.001) {
                        body._log("write step = " + deg.toFixed(1) + "°")
                        body.dev.servoStepDeg = deg
                    }
                }
            }
        }

        KIslandRow {
            label: qsTr("Scan range, °")

            DevSpin {
                from: 0; to: 360; stepSize: 1
                devValue: body.dev ? Math.round(body.dev.servoRangeDeg) : 0
                writeBack: function(v) {
                    if (body.dev && Math.round(body.dev.servoRangeDeg) !== v) {
                        body._log("write scan range = " + v + "°")
                        body.dev.servoRangeDeg = v
                    }
                }
            }
        }

        KIslandRow {
            label: qsTr("Center, °")

            DevSpin {
                from: -180; to: 180; stepSize: 1
                devValue: body.dev ? Math.round(body.dev.servoCenterDeg) : 0
                writeBack: function(v) {
                    if (body.dev && Math.round(body.dev.servoCenterDeg) !== v) {
                        body._log("write center = " + v + "°")
                        body.dev.servoCenterDeg = v
                    }
                }
            }
        }
    }

    KIsland {
        visible: !!body.dev

        KIslandRow {
            label: qsTr("More settings")
            interactive: true
            onClicked: body._advanced = !body._advanced

            DisclosureIndicator {
                expanded: body._advanced
                width: Math.round(10 * AppPalette.scale)
                height: width
            }
        }

        KIslandSection {
            label: qsTr("Calibration")
            open: body._advanced
        }

        KIslandRow {
            open: body._advanced
            label: qsTr("PWM min, µs")

            DevSpin {
                from: 500; to: 2500; stepSize: 10
                devValue: body.dev ? body.dev.servoPwmMinUs : 500
                writeBack: function(v) {
                    if (body.dev && body.dev.servoPwmMinUs !== v) {
                        body._log("write pwm min = " + v + "µs")
                        body.dev.servoPwmMinUs = v
                    }
                }
            }
        }

        KIslandRow {
            open: body._advanced
            label: qsTr("PWM max, µs")

            DevSpin {
                from: 500; to: 2500; stepSize: 10
                devValue: body.dev ? body.dev.servoPwmMaxUs : 2500
                writeBack: function(v) {
                    if (body.dev && body.dev.servoPwmMaxUs !== v) {
                        body._log("write pwm max = " + v + "µs")
                        body.dev.servoPwmMaxUs = v
                    }
                }
            }
        }

        KIslandRow {
            open: body._advanced
            label: qsTr("Servo angle range, °")

            DevSpin {
                from: 1; to: 360; stepSize: 1
                devValue: body.dev ? Math.round(body.dev.servoAngleRangeDeg) : 180
                writeBack: function(v) {
                    if (body.dev && Math.round(body.dev.servoAngleRangeDeg) !== v) {
                        body._log("write servo angle range = " + v + "°")
                        body.dev.servoAngleRangeDeg = v
                    }
                }
            }
        }

        KIslandSection {
            label: qsTr("PWM routing")
            open: body._advanced
        }

        KIslandRow {
            open: body._advanced
            label: qsTr("OUT1 (wired)")

            DevCombo {
                model: body._pwmTargets
                wantIndex: body.dev ? body.dev.pwmRouteOut1 : 0
                writeBack: function(i) {
                    if (body.dev && body.dev.pwmRouteOut1 !== i) {
                        body._log("write pwm route OUT1 = " + i)
                        body.dev.pwmRouteOut1 = i
                    }
                }
            }
        }

        KIslandRow {
            open: body._advanced
            label: qsTr("OUT2")

            DevCombo {
                model: body._pwmTargets
                wantIndex: body.dev ? body.dev.pwmRouteOut2 : 0
                writeBack: function(i) {
                    if (body.dev && body.dev.pwmRouteOut2 !== i) {
                        body._log("write pwm route OUT2 = " + i)
                        body.dev.pwmRouteOut2 = i
                    }
                }
            }
        }

        KIslandRow {
            open: body._advanced
            label: qsTr("OUT3")

            DevCombo {
                model: body._pwmTargets
                wantIndex: body.dev ? body.dev.pwmRouteOut3 : 0
                writeBack: function(i) {
                    if (body.dev && body.dev.pwmRouteOut3 !== i) {
                        body._log("write pwm route OUT3 = " + i)
                        body.dev.pwmRouteOut3 = i
                    }
                }
            }
        }
    }
}
