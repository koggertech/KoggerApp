import QtQuick 2.15
import kqml_types 1.0

Item {
    id: content

    property string rep: "value"
    property string label: ""
    property string value: ""
    property real k: 1.0
    property real gap: 0

    readonly property real _margin: Math.round(6 * k)
    readonly property real _minPx: Math.max(6, Math.round(7 * k))

    readonly property int _labelPx:    Math.max(_minPx, Math.round(content.height * 0.17))
    readonly property int _valuePx:    Math.max(_minPx, Math.round(content.height * 0.30))
    readonly property int _valueBigPx: Math.max(_minPx, Math.round(content.height * 0.45))

    readonly property bool _noData: content.value === "—"

    component ValueView : Item {
        id: vv
        property string text: ""
        property int pixelSize: 12
        property int minPx: 6
        property bool noData: false
        property int hAlign: Text.AlignHCenter

        Text {
            visible: !vv.noData
            anchors.fill: parent
            horizontalAlignment: vv.hAlign
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.NoWrap
            maximumLineCount: 2
            fontSizeMode: Text.Fit
            minimumPixelSize: vv.minPx
            font.pixelSize: vv.pixelSize
            font.bold: true
            elide: Text.ElideRight
            text: vv.text
            color: AppPalette.textStrong
        }
        Rectangle {
            visible: vv.noData
            width: Math.round(vv.pixelSize * 0.85)
            height: Math.max(2, Math.round(vv.pixelSize * 0.09))
            y: Math.round((vv.height - height) / 2)
            x: vv.hAlign === Text.AlignLeft ? 0
               : vv.hAlign === Text.AlignRight ? (vv.width - width)
               : Math.round((vv.width - width) / 2)
            radius: height / 2
            color: AppPalette.textStrong
        }
    }

    ValueView {
        visible: content.rep === "value"
        anchors.fill: parent
        anchors.margins: content._margin
        text: content.value
        pixelSize: content._valueBigPx
        minPx: content._minPx
        noData: content._noData
    }

    Item {
        visible: content.rep === "labelValueStacked"
        anchors.fill: parent
        anchors.margins: content._margin

        Text {
            id: stLabel
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: parent.height * 0.45
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            fontSizeMode: Text.Fit
            minimumPixelSize: content._minPx
            font.pixelSize: content._labelPx
            font.bold: true
            elide: Text.ElideRight
            text: content.label
            color: AppPalette.textSecond
        }
        ValueView {
            anchors.top: stLabel.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            text: content.value
            pixelSize: content._valuePx
            minPx: content._minPx
            noData: content._noData
        }
    }

    Item {
        visible: content.rep === "labelValueRow"
        anchors.fill: parent
        anchors.margins: content._margin

        Text {
            id: rowLabel
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: (parent.width - content.gap) / 2
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
            maximumLineCount: 3
            fontSizeMode: Text.Fit
            minimumPixelSize: content._minPx
            font.pixelSize: content._labelPx
            font.bold: true
            elide: Text.ElideRight
            text: content.label
            color: AppPalette.textSecond
        }
        ValueView {
            anchors.left: rowLabel.right
            anchors.leftMargin: content.gap
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            text: content.value
            pixelSize: content._valuePx
            minPx: content._minPx
            noData: content._noData
            hAlign: Text.AlignLeft
        }
    }
}
