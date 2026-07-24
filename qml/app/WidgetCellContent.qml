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

    Text {
        visible: content.rep === "value"
        anchors.fill: parent
        anchors.margins: content._margin
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        fontSizeMode: Text.Fit
        minimumPixelSize: content._minPx
        font.pixelSize: Math.round(content.height * 0.6)
        font.bold: true
        elide: Text.ElideRight
        text: content.value
        color: AppPalette.textStrong
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
            font.pixelSize: Math.round(content.height * 0.42)
            font.bold: true
            elide: Text.ElideRight
            text: content.label
            color: AppPalette.textSecond
        }
        Text {
            anchors.top: stLabel.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            verticalAlignment: Text.AlignVCenter
            fontSizeMode: Text.Fit
            minimumPixelSize: content._minPx
            font.pixelSize: Math.round(content.height * 0.55)
            font.bold: true
            elide: Text.ElideRight
            text: content.value
            color: AppPalette.textStrong
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
            maximumLineCount: 2
            fontSizeMode: Text.Fit
            minimumPixelSize: content._minPx
            font.pixelSize: Math.round(content.height * 0.42)
            font.bold: true
            elide: Text.ElideRight
            text: content.label
            color: AppPalette.textSecond
        }
        Text {
            anchors.left: rowLabel.right
            anchors.leftMargin: content.gap
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            verticalAlignment: Text.AlignVCenter
            fontSizeMode: Text.Fit
            minimumPixelSize: content._minPx
            font.pixelSize: Math.round(content.height * 0.5)
            font.bold: true
            elide: Text.ElideRight
            text: content.value
            color: AppPalette.textStrong
        }
    }
}
