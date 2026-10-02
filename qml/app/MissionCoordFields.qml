import QtQuick 2.15
import kqml_types 1.0

Item {
    id: root

    property real lat: NaN
    property real lon: NaN
    property int decimals: 6

    signal committed(real lat, real lon)

    implicitHeight: Tokens.controlHMd
    height: implicitHeight

    component CoordBox: Rectangle {
        id: box

        property real value: NaN
        property string label: ""
        property real minValue: -90
        property real maxValue: 90

        signal edited(real v)

        height: Tokens.controlHMd
        radius: Tokens.radiusMd
        color: AppPalette.card
        border.color: input.activeFocus ? AppPalette.borderFocus : AppPalette.border
        border.width: 1

        function formatted() { return isFinite(box.value) ? Number(box.value).toFixed(root.decimals) : "" }

        onValueChanged: if (!input.activeFocus) input.text = formatted()

        Text {
            id: lbl
            anchors.left: parent.left
            anchors.leftMargin: Tokens.spaceSm
            anchors.verticalCenter: parent.verticalCenter
            text: box.label
            color: AppPalette.textMuted
            font.pixelSize: Tokens.fontSm
        }

        TextInput {
            id: input
            anchors.left: lbl.right
            anchors.leftMargin: Tokens.spaceSm
            anchors.right: parent.right
            anchors.rightMargin: Tokens.spaceSm
            anchors.verticalCenter: parent.verticalCenter
            color: AppPalette.textStrong
            font.pixelSize: Tokens.fontBase
            selectByMouse: true
            clip: true
            inputMethodHints: Qt.ImhFormattedNumbersOnly
            property bool dirty: false
            validator: RegularExpressionValidator { regularExpression: /^-?\d{0,3}([.,]\d{0,8})?$/ }
            Component.onCompleted: text = box.formatted()
            onTextEdited: dirty = true
            onActiveFocusChanged: {
                if (activeFocus) return
                dirty = false
                text = box.formatted()
            }
            onEditingFinished: {
                var v = parseFloat(text.replace(",", "."))
                if (dirty && isFinite(v) && v >= box.minValue && v <= box.maxValue && Math.abs(v - box.value) > 1e-9) box.edited(v)
                else text = box.formatted()
                dirty = false
            }
        }
    }

    Row {
        anchors.fill: parent
        spacing: Tokens.spaceSm

        CoordBox {
            width: (parent.width - Tokens.spaceSm) / 2
            label: qsTr("Lat")
            value: root.lat
            minValue: -90
            maxValue: 90
            onEdited: function(v) { root.committed(v, root.lon) }
        }
        CoordBox {
            width: (parent.width - Tokens.spaceSm) / 2
            label: qsTr("Lon")
            value: root.lon
            minValue: -180
            maxValue: 180
            onEdited: function(v) { root.committed(root.lat, v) }
        }
    }
}
