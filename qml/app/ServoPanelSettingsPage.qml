import QtQuick 2.15
import kqml_types 1.0

Column {
    id: page

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

    Text {
        width: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("Servo scanner control: sweep, step, centre and the live angle.")
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontSm
    }

    KSwitch {
        width: parent.width
        text: qsTr("Show automatically")
        toolTipText: qsTr("Brings the panel up when a device with servo firmware connects.")
        checked: !!(page.store && page.store.servoPanelAutoShow)
        onToggled: if (page.store) page.store.servoPanelAutoShow = checked
    }

    Column {
        width: parent.width
        spacing: Tokens.spaceXs

        Text {
            text: qsTr("Background transparency")
            color: AppPalette.text
            font.pixelSize: Tokens.fontBase
        }

        KSlider {
            width: parent.width
            from: 0
            to: 100
            stepSize: 1
            showValueTip: false
            value: page.store ? page.store.servoPanelTransparency : 0
            valueSuffix: "%"
            onValueModified: function(v) { if (page.store) page.store.servoPanelTransparency = Math.round(v) }
        }
    }
}
