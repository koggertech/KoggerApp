import QtQuick 2.15
import kqml_types 1.0

Column {
    id: page

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceMd

    readonly property int valueLabelW: Math.round(60 * AppPalette.scale)

    // A user drag severs the slider `value:` binding — re-assert from the store.
    Connections {
        target: page.store
        ignoreUnknownSignals: true
        function onExtraInfoOpacityChanged() { opacitySlider.value = page.store.extraInfoOpacity }
    }

    // One switch per displayed field. Visibility is persisted in the store field map
    // (store.extraInfoFieldEnabled / setExtraInfoFieldEnabled). Tooltip per field via
    // KSwitch.toolTipText. Labels/tips are inline qsTr so they retranslate live.
    component FieldGroup: Column {
        property string title: ""
        property var fields: []
        width: parent ? parent.width : implicitWidth
        spacing: Tokens.spaceMd

        Text {
            visible: title.length > 0
            text: title
            color: AppPalette.textStrong
            font.pixelSize: Tokens.fontLg
            font.bold: true
            leftPadding: Tokens.spaceXxs
        }
        Repeater {
            model: fields
            delegate: KSwitch {
                required property var modelData
                width: parent.width
                visible: modelData.vis === undefined ? true : modelData.vis
                text: modelData.label
                toolTipText: modelData.tip
                checked: page.store ? page.store.extraInfoFieldEnabled(modelData.key) : false
                onToggled: if (page.store) page.store.setExtraInfoFieldEnabled(modelData.key, checked)
            }
        }
    }

    Text {
        width: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("The info panel is a floating info window. Turn on the fields below — a value appears when its data is available.")
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontSm
    }

    KButton {
        width: parent.width
        height: Tokens.controlHMd
        fontPixelSize: Tokens.fontLg
        text: qsTr("Defaults")
        onClicked: if (page.store) page.store.resetExtraInfoFields()
    }

    KSwitch {
        width: parent.width
        text: qsTr("Show info panel")
        checked: page.store ? page.store.extraInfoVisible : false
        onToggled: if (page.store) page.store.extraInfoVisible = checked
    }

    KSwitch {
        width: parent.width
        text: qsTr("Transparency")
        checked: page.store ? page.store.extraInfoTransparencyEnabled : false
        onToggled: if (page.store) page.store.extraInfoTransparencyEnabled = checked
    }

    Row {
        width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd
        visible: page.store ? page.store.extraInfoTransparencyEnabled : false

        Text {
            id: opacityLabel
            text: qsTr("Opacity:")
            color: AppPalette.textStrong; font.pixelSize: Tokens.fontLg
            anchors.verticalCenter: parent.verticalCenter
        }

        KSlider {
            id: opacitySlider
            showValueTip: false
            width: parent.width - opacityLabel.width - page.valueLabelW - 2 * Tokens.spaceMd
            anchors.verticalCenter: parent.verticalCenter
            from: 0; to: 100; stepSize: 5
            value: page.store ? page.store.extraInfoOpacity : 50
            valueSuffix: "%"
            onValueModified: function(v) { if (page.store) page.store.extraInfoOpacity = v }
        }

        Text {
            width: page.valueLabelW
            horizontalAlignment: Text.AlignRight
            anchors.verticalCenter: parent.verticalCenter
            text: (page.store ? Math.round(page.store.extraInfoOpacity) : 50) + "%"
            color: AppPalette.text; font.pixelSize: Tokens.fontLg
        }
    }

    // Flat list (no section header). Order mirrors the panel.
    FieldGroup {
        fields: [
            { key: "time",      label: qsTr("Time"),          tip: qsTr("Local device clock."), vis: (page.store ? page.store.systemTimeValid : true) },
            { key: "depth",     label: qsTr("Total depth"),   tip: qsTr("Last depth — from the rangefinder or bottom track.") },
            { key: "rfDepth",   label: qsTr("Rangefinder"),   tip: qsTr("Depth from the sonar.") },
            { key: "btDepth",   label: qsTr("Bottom track"),  tip: qsTr("Depth obtained by post-processing in the app.") },
            { key: "speed",     label: qsTr("Speed"),         tip: qsTr("Boat ground speed derived from GNSS positions, in km/h.") },
            { key: "apSpeed",   label: qsTr("Ground speed"),  tip: qsTr("Autopilot horizontal ground speed (MAVLink), in m/s.") },
            { key: "coord",     label: qsTr("Coordinate"),    tip: qsTr("Boat position (latitude and longitude) of the latest GNSS fix.") },
            { key: "selPoint",  label: qsTr("Selected point"), tip: qsTr("Distance and angle from the boat to the selected point.") },
            { key: "temp",      label: qsTr("Temperature"),   tip: qsTr("Water temperature measured by the sonar, in °C.") },
            { key: "apVoltage", label: qsTr("Battery voltage"), tip: qsTr("Autopilot battery voltage (MAVLink), in volts.") },
            { key: "apCurrent", label: qsTr("Current"),       tip: qsTr("Autopilot battery current draw (MAVLink), in amperes.") },
            { key: "apMode",    label: qsTr("Flight mode"),   tip: qsTr("Autopilot flight/drive mode number (MAVLink custom mode).") },
            { key: "apArm",     label: qsTr("Arm state"),     tip: qsTr("Whether the autopilot is armed (MAVLink).") }
        ]
    }

    // ── SimpleNavV2 Navigation group: HIDDEN — uncomment block to restore [nav-settings] ──
    /*
    FieldGroup {
        title: qsTr("Navigation")
        fields: [
            { key: "navFix",      label: qsTr("GNSS fix type"),  tip: qsTr("GNSS fix type from the SimpleNavV2 frame (0 = no fix).") },
            { key: "navSats",     label: qsTr("Satellites"),     tip: qsTr("Number of satellites used in the fix.") },
            { key: "navTime",     label: qsTr("UTC time"),       tip: qsTr("Fix time from the SimpleNavV2 frame (Unix seconds).") },
            { key: "navOffset",   label: qsTr("Time offset"),    tip: qsTr("Sub-second offset of the fix time, in milliseconds.") },
            { key: "navLat",      label: qsTr("Nav latitude"),   tip: qsTr("High-precision latitude from the SimpleNavV2 frame.") },
            { key: "navLon",      label: qsTr("Nav longitude"),  tip: qsTr("High-precision longitude from the SimpleNavV2 frame.") },
            { key: "navCourse",   label: qsTr("Ground course"),  tip: qsTr("Course over ground from the SimpleNavV2 frame, in degrees.") },
            { key: "navVelocity", label: qsTr("Ground velocity"),tip: qsTr("Ground velocity from the SimpleNavV2 frame, in m/s.") },
            { key: "navYaw",      label: qsTr("Yaw"),            tip: qsTr("Yaw / heading from the SimpleNavV2 frame, in degrees.") },
            { key: "navPitch",    label: qsTr("Pitch"),          tip: qsTr("Pitch from the SimpleNavV2 frame, in degrees.") },
            { key: "navRoll",     label: qsTr("Roll"),           tip: qsTr("Roll from the SimpleNavV2 frame, in degrees.") }
        ]
    }
    */

    // ── Boat status group: HIDDEN — uncomment block to restore [bs-settings] ──
    /*
    FieldGroup {
        title: qsTr("Boat status")
        fields: [
            { key: "bsBatBoat",   label: qsTr("Battery (boat)"),   tip: qsTr("Battery charge of the boat unit, in percent.") },
            { key: "bsBatBridge", label: qsTr("Battery (bridge)"), tip: qsTr("Battery charge of the shore/bridge unit, in percent.") },
            { key: "bsSigBoat",   label: qsTr("Signal (boat)"),    tip: qsTr("Radio link signal quality at the boat unit, in percent.") },
            { key: "bsSigBridge", label: qsTr("Signal (bridge)"),  tip: qsTr("Radio link signal quality at the shore/bridge unit, in percent.") }
        ]
    }
    */
}
