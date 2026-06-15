import QtQuick 2.15
import kqml_types 1.0

Column {
    id: page

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

    readonly property int valueLabelW: Math.round(60 * AppPalette.scale)
    readonly property int labelW: Math.round(92 * AppPalette.scale)
    readonly property real gainNear: page.store ? page.store.tgcGainNear : 50
    readonly property real gainFar: page.store ? page.store.tgcGainFar : 250

    // A user drag severs the `value:` binding (Qt Slider sets value imperatively).
    // Re-assert from the store so label-reset / UI-import move the thumb too.
    Connections {
        target: page.store
        ignoreUnknownSignals: true
        function onTgcGainNearChanged()   { tgcGainNearSlider.value     = page.store.tgcGainNear }
        function onTgcGainFarChanged()    { tgcGainFarSlider.value      = page.store.tgcGainFar }
        function onTgcCompensateChanged() { tgcCompensateSwitch.checked = page.store.tgcCompensate }
    }

    Text {
        width: parent.width
        text: qsTr("Time Gain Compensation — image brightness adjustment by depth.")
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontSm
        wrapMode: Text.WordWrap
        bottomPadding: Tokens.spaceXs
    }

    Row {
        width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd

        Text {
            text: qsTr("Near gain:")
            color: AppPalette.textSecond; font.pixelSize: Tokens.fontMd
            width: page.labelW
            anchors.verticalCenter: parent.verticalCenter
            elide: Text.ElideRight

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: if (page.store) page.store.tgcGainNear = 100
            }
        }

        KSlider {
            id: tgcGainNearSlider
            width: parent.width - page.labelW - page.valueLabelW - 2 * Tokens.spaceMd
            anchors.verticalCenter: parent.verticalCenter
            from: 0; to: 500; stepSize: 1
            value: page.gainNear
            valueSuffix: "%"
            onValueModified: function(v) { if (page.store) page.store.tgcGainNear = v }
        }

        Text {
            width: page.valueLabelW
            horizontalAlignment: Text.AlignRight
            anchors.verticalCenter: parent.verticalCenter
            text: Math.round(page.gainNear) + "%"
            color: AppPalette.text; font.pixelSize: Tokens.fontMd
        }
    }

    Row {
        width: parent.width; height: Tokens.controlHMd; spacing: Tokens.spaceMd

        Text {
            text: qsTr("Far gain:")
            color: AppPalette.textSecond; font.pixelSize: Tokens.fontMd
            width: page.labelW
            anchors.verticalCenter: parent.verticalCenter
            elide: Text.ElideRight

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: if (page.store) page.store.tgcGainFar = 100
            }
        }

        KSlider {
            id: tgcGainFarSlider
            width: parent.width - page.labelW - page.valueLabelW - 2 * Tokens.spaceMd
            anchors.verticalCenter: parent.verticalCenter
            from: 0; to: 1000; stepSize: 1
            value: page.gainFar
            valueSuffix: "%"
            onValueModified: function(v) { if (page.store) page.store.tgcGainFar = v }
        }

        Text {
            width: page.valueLabelW
            horizontalAlignment: Text.AlignRight
            anchors.verticalCenter: parent.verticalCenter
            text: Math.round(page.gainFar) + "%"
            color: AppPalette.text; font.pixelSize: Tokens.fontMd
        }
    }

    Canvas {
        id: tgcCurveCanvas
        width: parent.width
        height: Math.round(100 * AppPalette.scale)

        Connections {
            target: page.store
            ignoreUnknownSignals: true
            function onTgcGainNearChanged() { tgcCurveCanvas.requestPaint() }
            function onTgcGainFarChanged()  { tgcCurveCanvas.requestPaint() }
        }

        onPaint: {
            var ctx = getContext("2d")
            if (!ctx)
                return
            var w = width
            var h = height

            ctx.fillStyle = AppPalette.bg
            ctx.fillRect(0, 0, w, h)

            var gNear = page.gainNear / 100.0
            var gFar  = page.gainFar / 100.0

            var yMax = Math.max(gNear, gFar, 1.0) * 1.15
            if (yMax < 0.5) yMax = 0.5

            function yFor(g) { return h - (g / yMax) * h }

            ctx.strokeStyle = AppPalette.border
            ctx.lineWidth = 1
            ctx.beginPath()
            ctx.moveTo(0, h - 0.5)
            ctx.lineTo(w, h - 0.5)
            ctx.stroke()

            var y100 = yFor(1.0)
            ctx.strokeStyle = AppPalette.text
            ctx.globalAlpha = 0.35
            if (ctx.setLineDash) ctx.setLineDash([3, 3])
            ctx.beginPath()
            ctx.moveTo(0, y100)
            ctx.lineTo(w, y100)
            ctx.stroke()
            if (ctx.setLineDash) ctx.setLineDash([])
            ctx.globalAlpha = 1.0

            ctx.strokeStyle = "#F07000"
            ctx.lineWidth = 2
            ctx.beginPath()
            ctx.moveTo(0, yFor(gNear))
            ctx.lineTo(w, yFor(gFar))
            ctx.stroke()

            ctx.fillStyle = AppPalette.text
            ctx.globalAlpha = 0.6
            ctx.font = "10px sans-serif"
            ctx.fillText("100%", 4, Math.max(y100 - 2, 10))
            ctx.globalAlpha = 1.0
        }
    }

    KSwitch {
        id: tgcCompensateSwitch
        width: parent.width
        text: qsTr("Compensate")
        checked: page.store ? page.store.tgcCompensate : false
        onToggled: if (page.store) page.store.tgcCompensate = checked
    }
}
