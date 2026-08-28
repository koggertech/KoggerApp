import QtQuick 2.15
import QtQuick.Layouts 1.15
import kqml_types 1.0

Column {
    id: page

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceMd

    readonly property string contentId: page.store ? page.store.videoSettingsContentId : ""

    readonly property var surface: {
        if (!page.store || typeof videoStreams === "undefined" || !videoStreams)
            return null
        var map = page.store.videoSourceByContent
        var uuid = (map && page.contentId.length) ? map[page.contentId] : ""
        if (!uuid)
            return null
        var all = videoStreams.streams
        for (var i = 0; i < all.length; ++i) {
            if (String(all[i].uuid) === String(uuid))
                return all[i]
        }
        return null
    }

    readonly property var stream: (page.surface && typeof videoStreams !== "undefined" && videoStreams)
                                   ? videoStreams.streamFor(String(page.surface.uuid))
                                   : null

    component SectionLabel: Text {
        color: AppPalette.textStrong
        font.pixelSize: Tokens.fontLg
        font.bold: true
        topPadding: Tokens.spaceXs
        leftPadding: Tokens.spaceXxs
    }

    SectionLabel { text: qsTr("Image:") }

    KTabBar {
        width: parent.width
        fontPixelSize: Tokens.fontLg
        trackColor: AppPalette.bgDeep
        options: [
            { label: qsTr("Fit"), value: 0 },
            { label: qsTr("Crop"), value: 1 },
            { label: qsTr("Stretch"), value: 2 }
        ]
        currentValue: page.store ? page.store.videoFillForContent(page.contentId) : 0
        onValueSelected: function(value) {
            if (page.store)
                page.store.setVideoFillForContent(page.contentId, value)
        }
    }

    KSwitch {
        width: parent.width
        text: qsTr("Show resolution")
        checked: page.store ? page.store.videoResolutionVisibleForContent(page.contentId) : true
        onToggled: if (page.store) page.store.setVideoResolutionVisibleForContent(page.contentId, checked)
    }

    SectionLabel { text: qsTr("Stream:") }

    Column {
        width: parent.width
        spacing: Tokens.spaceSm

        component ReadoutRow: RowLayout {
            id: readoutRow
            property string label: ""
            property string value: ""
            width: parent ? parent.width : implicitWidth
            spacing: Tokens.spaceMd

            Text {
                text: readoutRow.label
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontBase
                Layout.alignment: Qt.AlignVCenter
            }
            Text {
                text: readoutRow.value
                color: AppPalette.text
                font.pixelSize: Tokens.fontBase
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideMiddle
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
            }
        }

        ReadoutRow {
            label: qsTr("Source")
            value: page.surface ? String(page.surface.label) : qsTr("Not selected")
        }

        ReadoutRow {
            label: qsTr("Resolution")
            value: (page.stream && page.stream.sourceWidth > 0)
                   ? page.stream.sourceWidth + "×" + page.stream.sourceHeight
                   : "—"
        }

        ReadoutRow {
            label: qsTr("State")
            value: {
                if (!page.stream)
                    return "—"
                if (page.stream.statusText.length)
                    return page.stream.statusText
                return page.stream.hasFrame ? qsTr("Streaming") : "—"
            }
        }
    }
}
