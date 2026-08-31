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
        text: qsTr("This panel configures and runs a calibration stand: the scan order, the inner "
                 + "and outer angles, the step and the dwell. The stand takes the whole "
                 + "configuration only as part of Start, so it is set and sent in the panel itself.")
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontSm
    }

    KSwitch {
        width: parent.width
        text: qsTr("Show automatically")
        toolTipText: qsTr("Brings the panel up when a device with a stand connects.")
        checked: !!(page.store && page.store.standPanelAutoShow)
        onToggled: if (page.store) page.store.standPanelAutoShow = checked
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
            value: page.store ? page.store.standPanelTransparency : 0
            valueSuffix: "%"
            onValueModified: function(v) { if (page.store) page.store.standPanelTransparency = Math.round(v) }
        }
    }
}
