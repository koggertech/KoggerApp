import QtQuick 2.15
import kqml_types 1.0

Column {
    id: page

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

    KSwitch {
        width: parent.width
        text: qsTr("Show console")
        checked: theme ? theme.consoleVisible : false
        onToggled: if (theme) theme.consoleVisible = checked
    }

    KSwitch {
        width: parent.width
        text: qsTr("Colour marking")
        toolTipText: qsTr("Highlight log syntax with the app theme's colours")
        checked: page.store ? page.store.consoleColorize : true
        onToggled: if (page.store) page.store.consoleColorize = checked
    }

    Column {
        width: parent.width
        spacing: Tokens.spaceSm

        Row {
            width: parent.width
            spacing: Tokens.spaceMd

            Text {
                text: qsTr("Keep last lines:")
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontBase
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: page.store ? Math.round(page.store.consoleMaxRows) : ""
                color: AppPalette.text
                font.pixelSize: Tokens.fontBase; font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        KSlider {
            id: rowsSlider
            width: parent.width
            from: 50; to: 4000; stepSize: 50
            showValueTip: false
            value: page.store ? page.store.consoleMaxRows : 500
            onValueModified: function(v) { if (page.store) page.store.consoleMaxRows = v }
        }

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            text: qsTr("Ring buffer — the console keeps only the newest lines; older ones are dropped.")
            color: AppPalette.textMuted
            font.pixelSize: Tokens.fontSm
        }
    }
}
