import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

DevSettingsBox {
    id: control
    isActive: !!(dev && dev.isServoSupport)

    readonly property var pwmTargets: [qsTr("Off"), qsTr("ServoScan")]

    ColumnLayout {
        id: columnItem
        spacing: 24
        Layout.margins: 24

        ParamGroup {
            groupName: qsTr("Control")

            RowLayout {
                CCheck {
                    id: enabledCheck
                    text: qsTr("Enabled (sweep on CHART trigger)")
                    checked: !!(dev && dev.servoEnabled)
                    onCheckedChanged: {
                        if (dev && dev.servoEnabled !== checked)
                            dev.servoEnabled = checked
                    }
                }
            }

            RowLayout {
                CText {
                    text: qsTr("Current angle:")
                }
                CText {
                    text: dev ? dev.servoCurrentAngleDeg.toFixed(2) + qsTr("°") : "—"
                }
            }

            RowLayout {
                CCheck {
                    id: reverseCheck
                    text: qsTr("Reverse mapping")
                    checked: !!(dev && dev.servoReverse)
                    onCheckedChanged: {
                        if (dev && dev.servoReverse !== checked)
                            dev.servoReverse = checked
                    }
                }
            }
        }

        ParamGroup {
            groupName: qsTr("Calibration")

            RowLayout {
                CText { text: qsTr("PWM min, µs") }
                SpinBoxCustom {
                    from: 500
                    to: 2500
                    stepSize: 10
                    devValue: dev ? dev.servoPwmMinUs : 500
                    onValueChanged: {
                        if (dev && value !== dev.servoPwmMinUs)
                            dev.servoPwmMinUs = value
                    }
                }
            }

            RowLayout {
                CText { text: qsTr("PWM max, µs") }
                SpinBoxCustom {
                    from: 500
                    to: 2500
                    stepSize: 10
                    devValue: dev ? dev.servoPwmMaxUs : 2500
                    onValueChanged: {
                        if (dev && value !== dev.servoPwmMaxUs)
                            dev.servoPwmMaxUs = value
                    }
                }
            }

            RowLayout {
                CText { text: qsTr("Servo angle range, °") }
                SpinBoxCustom {
                    from: 1
                    to: 360
                    stepSize: 1
                    devValue: dev ? Math.round(dev.servoAngleRangeDeg) : 180
                    onValueChanged: {
                        if (dev && Math.round(dev.servoAngleRangeDeg) !== value)
                            dev.servoAngleRangeDeg = value
                    }
                }
            }
        }

        ParamGroup {
            groupName: qsTr("Scan")

            RowLayout {
                CText { text: qsTr("Step, °") }
                SpinBoxCustom {
                    from: -360
                    to: 360
                    stepSize: 1
                    devValue: dev ? Math.round(dev.servoStepDeg) : 5
                    onValueChanged: {
                        if (dev && Math.round(dev.servoStepDeg) !== value)
                            dev.servoStepDeg = value
                    }
                }
            }

            RowLayout {
                CText { text: qsTr("Scan range, °") }
                SpinBoxCustom {
                    from: 0
                    to: 360
                    stepSize: 1
                    devValue: dev ? Math.round(dev.servoRangeDeg) : 180
                    onValueChanged: {
                        if (dev && Math.round(dev.servoRangeDeg) !== value)
                            dev.servoRangeDeg = value
                    }
                }
            }

            RowLayout {
                CText { text: qsTr("Center, °") }
                SpinBoxCustom {
                    from: -180
                    to: 180
                    stepSize: 1
                    devValue: dev ? Math.round(dev.servoCenterDeg) : 0
                    onValueChanged: {
                        if (dev && Math.round(dev.servoCenterDeg) !== value)
                            dev.servoCenterDeg = value
                    }
                }
            }
        }

        ParamGroup {
            groupName: qsTr("PWM routing")

            RowLayout {
                CText { text: qsTr("OUT1 (wired)") }
                CCombo {
                    model: control.pwmTargets
                    currentIndex: dev ? dev.pwmRouteOut1 : 0
                    onActivated: function(index) {
                        if (dev && dev.pwmRouteOut1 !== index)
                            dev.pwmRouteOut1 = index
                    }
                }
            }

            RowLayout {
                CText { text: qsTr("OUT2") }
                CCombo {
                    model: control.pwmTargets
                    currentIndex: dev ? dev.pwmRouteOut2 : 0
                    onActivated: function(index) {
                        if (dev && dev.pwmRouteOut2 !== index)
                            dev.pwmRouteOut2 = index
                    }
                }
            }

            RowLayout {
                CText { text: qsTr("OUT3") }
                CCombo {
                    model: control.pwmTargets
                    currentIndex: dev ? dev.pwmRouteOut3 : 0
                    onActivated: function(index) {
                        if (dev && dev.pwmRouteOut3 !== index)
                            dev.pwmRouteOut3 = index
                    }
                }
            }
        }
    }
}
