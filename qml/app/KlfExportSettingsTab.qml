import QtQuick 2.15
import kqml_types 1.0

Column {
    id: page

    required property var store
    property var echograms: []

    readonly property int epochCount: (typeof core !== "undefined" && core) ? core.klfEpochIndexSize : 0

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceSm

    component Spacer: Item {
        property int size: Tokens.spaceMd
        width: page.width
        height: size
    }

    component EpochBoundSlider: Item {
        id: bound
        property string caption: ""
        property real captionWidth: boundCaption.implicitWidth
        readonly property real captionImplicitWidth: boundCaption.implicitWidth
        property alias to: boundSlider.to
        property alias value: boundSlider.value
        signal valueModified(real val)

        width: parent ? parent.width : 0
        height: boundSlider.implicitHeight

        Text {
            id: boundCaption
            width: bound.captionWidth
            anchors.verticalCenter: parent.verticalCenter
            text: bound.caption
            color: AppPalette.textSecond
            font.pixelSize: Tokens.fontBase
        }
        KSlider {
            id: boundSlider
            anchors.left: boundCaption.right
            anchors.leftMargin: Tokens.spaceSm
            anchors.right: boundValue.left
            anchors.rightMargin: Tokens.spaceSm
            anchors.verticalCenter: parent.verticalCenter
            from: 0
            stepSize: 1
            onValueModified: function(val) { bound.valueModified(val) }
        }
        TextMetrics {
            id: boundValueMetrics
            font: boundValue.font
            text: String(Math.round(bound.to)).replace(/\d/g, "8")
        }
        Text {
            id: boundValue
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: Math.ceil(boundValueMetrics.advanceWidth)
            text: Math.round(boundSlider.value)
            color: AppPalette.textStrong
            font.pixelSize: Tokens.fontBase
            horizontalAlignment: Text.AlignRight
        }
    }

    function resetBounds() {
        fromSlider.value = 0
        toSlider.value = Math.max(0, page.epochCount - 1)
    }

    function takeVisibleRange(plot) {
        if (!plot)
            return
        var a = plot.firstVisibleEpochIndex()
        var b = plot.lastVisibleEpochIndex()
        if (a < 0 || b < 0)
            return
        fromSlider.value = Math.min(a, b)
        toSlider.value = Math.max(a, b)
    }

    onEpochCountChanged: Qt.callLater(page.resetBounds)
    Component.onCompleted: if (epochCount > 0) Qt.callLater(page.resetBounds)

    Text {
        width: parent.width
        wrapMode: Text.WordWrap
        text: page.epochCount > 0
              ? qsTr("Save a range of epochs of the opened file as a new KLF file. The destination folder is set in the Export group.")
              : qsTr("Open a KLF file to save a fragment of it.")
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontSm
    }

    Spacer {}

    KIsland {
        visible: page.epochCount > 0

        KIslandRow {
            label: qsTr("Manual selection")
            caption: qsTr("%1 of %2 epochs")
                     .arg(Math.round(toSlider.value - fromSlider.value) + 1)
                     .arg(page.epochCount)
            stacked: true

            Column {
                id: bounds
                readonly property real captionWidth: Math.max(fromSlider.captionImplicitWidth,
                                                              toSlider.captionImplicitWidth)
                width: parent.width

                EpochBoundSlider {
                    id: fromSlider
                    caption: qsTr("Start")
                    captionWidth: bounds.captionWidth
                    to: Math.max(0, page.epochCount - 1)
                    onValueModified: function(val) {
                        if (val > toSlider.value)
                            toSlider.value = val
                    }
                }

                EpochBoundSlider {
                    id: toSlider
                    caption: qsTr("End")
                    captionWidth: bounds.captionWidth
                    to: Math.max(0, page.epochCount - 1)
                    onValueModified: function(val) {
                        if (val < fromSlider.value)
                            fromSlider.value = val
                    }
                }
            }
        }

        KIslandRow {
            label: qsTr("By echogram")
            caption: page.echograms.length > 0 ? qsTr("Start and End from what the echogram shows")
                                               : qsTr("No echogram on screen")
            stacked: true

            Flow {
                width: parent.width
                spacing: Tokens.spaceSm

                Repeater {
                    model: page.echograms

                    KButton {
                        required property var modelData

                        height: Tokens.controlHLg
                        text: modelData.label
                        bold: false
                        normalBg: AppPalette.controlRaised
                        hoverBg: Qt.lighter(AppPalette.controlRaised, 1.2)
                        toolTipText: qsTr("Fill the bounds with the first and last epoch the echogram is showing")
                        onHoveredChanged: {
                            if (!page.store || page.echograms.length < 2)
                                return
                            if (hovered)
                                page.store.highlightedLeafId = modelData.key
                            else if (page.store.highlightedLeafId === modelData.key)
                                page.store.highlightedLeafId = -1
                        }
                        onClicked: page.takeVisibleRange(modelData.plot)
                    }
                }
            }
        }

        KIslandRow {
            stacked: true
            verticalPadding: Tokens.spaceMd

            KButton {
                width: parent.width
                height: Tokens.controlHLg
                text: qsTr("Save KLF fragment")
                accent: true
                onClicked: core.exportKlfFragment(Math.round(fromSlider.value),
                                                  Math.round(toSlider.value),
                                                  page.store ? page.store.exportFolderSource : "")
            }
        }
    }
}
