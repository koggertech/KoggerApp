import QtQuick 2.15
import kqml_types 1.0

// The whole editor for an acoustic-nodes panel, which is deliberately almost empty.
//
// There is nothing to lay out: the rows ARE the USBL plan, so what the panel shows is decided
// in the device settings, not here. Offering a column picker or a node filter would be a
// second place to configure the same thing and a second thing to keep in step with the plan.
//
// What is left is the one property every panel has — how much of the scene shows through.
Column {
    id: step

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

    Text {
        width: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("This panel shows one row per node in the USBL plan: its address, range and "
                 + "SNR, whether a request is out, how the last interrogation ended, which "
                 + "command that was, and how old the numbers are.")
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontSm
    }

    Text {
        width: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("Nodes are added and interrogated in device settings, under Acoustic nodes. "
                 + "The panel follows that plan — there is nothing to arrange here.")
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

    // Always enabled, unlike the grid's Save: a grid with no cells is an empty box, but a nodes
    // panel with no nodes is a panel waiting for a plan, and it says so on the scene.
    KButton {
        id: saveBtn
        width: parent.width
        text: qsTr("Save")
        normalBg: AppPalette.accentBg
        hoverBg: AppPalette.accentBg
        normalBorder: AppPalette.accentBorder
        onClicked: if (step.store) step.store.widgetDraftSave()
    }
}
