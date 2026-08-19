import QtQuick 2.15
import kqml_types 1.0

// The servo panel's editor, and like the nodes panel's there is nothing here to lay out: what
// the panel shows is whichever device on the bus answers the servo command, which is the bus's
// answer and not a choice to be made in advance.
Column {
    id: step

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

    Text {
        width: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("Controls the servo scanner of the device that has one. The panel finds it itself.")
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontSm
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
            value: step.store ? step.store.widgetDraftTransparency : 0
            valueSuffix: "%"
            onValueModified: function(v) { if (step.store) step.store.widgetDraftTransparency = Math.round(v) }
        }
    }

    KButton {
        width: parent.width
        text: qsTr("Save")
        normalBg: AppPalette.accentBg
        hoverBg: AppPalette.accentBg
        normalBorder: AppPalette.accentBorder
        onClicked: if (step.store) step.store.widgetDraftSave()
    }
}
