import QtQuick 2.15
import kqml_types 1.0

// The whole editor for a stand panel, which is nearly empty on purpose.
//
// There is nothing to lay out and nothing to pick: the scan is configured in the panel itself,
// because it is sent as one block by Start and belongs beside the button that sends it. A copy
// of the form here would be a second place to edit one configuration.
Column {
    id: step

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

    Text {
        width: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("This panel configures and runs a calibration stand: the scan order, the inner "
                 + "and outer shapes, how the inner axis travels, how often it fires and how long "
                 + "it waits.")
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontSm
    }

    Text {
        width: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("The scan is set on the panel itself, under the commands — the stand takes its "
                 + "whole configuration only as part of Start, so there is nothing to arrange here.")
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
