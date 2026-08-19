import QtQuick 2.15
import kqml_types 1.0

// The servo panel's content, with no panel around it: laid out at real token sizes, sizing
// itself vertically. Both surfaces that show it -- the panel on the scene and the editor
// overlay's centred preview -- instantiate this, so neither can drift from the other.
Column {
    id: body

    property real baseW: Math.round(300 * AppPalette.scale)
    property real ctlW: Math.round(118 * AppPalette.scale)

    readonly property bool _manualTesting: (typeof manualTesting !== "undefined") && manualTesting === true

    // MANUAL_TESTING builds only: one stand-in device so the panel can be laid out and looked at
    // with nothing on the bench. It is used ONLY while nothing real answers, so plugging a
    // device in takes over on the spot and the mock can never mask the real bus. Push a second
    // copy into `_devs` below to see the tabbed picker.
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

    // Sweeps the mock's angle while it is enabled, because a readout that never moves says
    // nothing about how it renders.
    property Timer _mockSweep: Timer {
        interval: 200
        repeat: true
        running: body._manualTesting && body.dev === body._mockDev && body._mockDev.servoEnabled
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

    // Selection survives the list being rebuilt, which it is on every topology change; an index
    // would silently move to another device when one ahead of it drops off the bus.
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

    // Tabs while they are still readable, a combo past that. KTabBar splits its width evenly, so
    // on a panel this narrow the fourth segment is a sliver -- and a picker nobody can read is
    // worse than one extra tap. Three is the count that still fits at the panel's base width.
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
        var n = (d.devName && d.devName.length > 0) ? d.devName : "#"
        return d.devSN > 0 ? n + " · " + d.devSN : n
    }

    // Reading `groups` is what makes this re-evaluate: groupForDevice is an invokable, and the
    // model rebuilds the whole list when the topology changes.
    function _linkLabelFor(d) {
        if (!d)
            return ""
        // The mock is not a DevQProperty, so it has no group to look up.
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

    // The device pushes its current values back on every ack and after every reconnect, so a
    // control bound straight to the property would fight the user's next click. Each of these
    // takes the device value as a WANT and only writes when the user actually moved it.
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
        title: qsTr("Servo")

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
                writeBack: function(v) { if (body.dev && body.dev.servoEnabled !== v) body.dev.servoEnabled = v }
            }
        }

        KIslandRow {
            label: qsTr("Current angle")

            Text {
                text: body.dev ? body.dev.servoCurrentAngleDeg.toFixed(2) + "°" : "—"
                color: AppPalette.textStrong
                font.pixelSize: Tokens.fontLg
                font.bold: true
            }
        }

        KIslandRow {
            label: qsTr("Reverse mapping")

            DevSwitch {
                wantChecked: !!(body.dev && body.dev.servoReverse)
                writeBack: function(v) { if (body.dev && body.dev.servoReverse !== v) body.dev.servoReverse = v }
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
                    if (Math.abs(body.dev.servoStepDeg - deg) > 0.001)
                        body.dev.servoStepDeg = deg
                }
            }
        }

        KIslandRow {
            label: qsTr("Scan range, °")

            DevSpin {
                from: 0; to: 360; stepSize: 1
                devValue: body.dev ? Math.round(body.dev.servoRangeDeg) : 0
                writeBack: function(v) {
                    if (body.dev && Math.round(body.dev.servoRangeDeg) !== v)
                        body.dev.servoRangeDeg = v
                }
            }
        }

        KIslandRow {
            label: qsTr("Center, °")

            DevSpin {
                from: -180; to: 180; stepSize: 1
                devValue: body.dev ? Math.round(body.dev.servoCenterDeg) : 0
                writeBack: function(v) {
                    if (body.dev && Math.round(body.dev.servoCenterDeg) !== v)
                        body.dev.servoCenterDeg = v
                }
            }
        }
    }

    // Calibration is the servo model's spec and the routing is wiring: both are set once per
    // hardware build, and neither belongs in front of an operator steering a survey.
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
                    if (body.dev && body.dev.servoPwmMinUs !== v)
                        body.dev.servoPwmMinUs = v
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
                    if (body.dev && body.dev.servoPwmMaxUs !== v)
                        body.dev.servoPwmMaxUs = v
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
                    if (body.dev && Math.round(body.dev.servoAngleRangeDeg) !== v)
                        body.dev.servoAngleRangeDeg = v
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
                writeBack: function(i) { if (body.dev && body.dev.pwmRouteOut1 !== i) body.dev.pwmRouteOut1 = i }
            }
        }

        KIslandRow {
            open: body._advanced
            label: qsTr("OUT2")

            DevCombo {
                model: body._pwmTargets
                wantIndex: body.dev ? body.dev.pwmRouteOut2 : 0
                writeBack: function(i) { if (body.dev && body.dev.pwmRouteOut2 !== i) body.dev.pwmRouteOut2 = i }
            }
        }

        KIslandRow {
            open: body._advanced
            label: qsTr("OUT3")

            DevCombo {
                model: body._pwmTargets
                wantIndex: body.dev ? body.dev.pwmRouteOut3 : 0
                writeBack: function(i) { if (body.dev && body.dev.pwmRouteOut3 !== i) body.dev.pwmRouteOut3 = i }
            }
        }
    }
}
