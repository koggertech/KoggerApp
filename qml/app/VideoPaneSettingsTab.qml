import QtQuick 2.15
import QtQuick.Layouts 1.15
import kqml_types 1.0

Column {
    id: page

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceMd

    readonly property string contentId: page.store ? page.store.videoSettingsContentId : ""

    readonly property string sourceUuid: {
        if (!page.store || !page.contentId.length)
            return ""
        var map = page.store.videoSourceByContent
        var uuid = map ? map[page.contentId] : ""
        return uuid ? String(uuid) : ""
    }

    readonly property var surface: {
        if (!page.sourceUuid.length || typeof videoStreams === "undefined" || !videoStreams)
            return null
        var all = videoStreams.streams || []
        for (var i = 0; i < all.length; ++i) {
            if (String(all[i].uuid) === page.sourceUuid)
                return all[i]
        }
        return null
    }

    readonly property var liveDescriptors: {
        var out = []
        if (typeof videoStreams === "undefined" || !videoStreams)
            return out
        var all = videoStreams.streams || []
        for (var i = 0; i < all.length; ++i) {
            if (all[i].open)
                out.push(all[i])
        }
        return out
    }

    readonly property var selectableDescriptors: {
        var out = [{ uuid: "", label: qsTr("Not selected") }]
        for (var i = 0; i < page.liveDescriptors.length; ++i)
            out.push(page.liveDescriptors[i])
        return out
    }

    readonly property var selectorLabels: {
        var out = []
        for (var i = 0; i < page.selectableDescriptors.length; ++i)
            out.push(String(page.selectableDescriptors[i].label))
        return out
    }

    readonly property int selectorIndex: {
        for (var i = 0; i < page.selectableDescriptors.length; ++i) {
            if (String(page.selectableDescriptors[i].uuid) === page.sourceUuid)
                return i
        }
        return 0
    }

    function pickSource(index) {
        if (!page.store || index < 0 || index >= page.selectableDescriptors.length)
            return
        page.store.setVideoSourceForContent(page.contentId,
                                            String(page.selectableDescriptors[index].uuid))
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

    SectionLabel { text: qsTr("Stream:") }

    Column {
        width: parent.width
        spacing: Tokens.spaceSm

        Rectangle {
            width: parent.width
            implicitHeight: Math.round(38 * AppPalette.scale)
            radius: Tokens.radiusLg
            color: AppPalette.rowRaised

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Math.round(10 * AppPalette.scale)
                anchors.rightMargin: Math.round(10 * AppPalette.scale)
                spacing: Tokens.spaceXs

                Text {
                    text: qsTr("Source")
                    color: AppPalette.isDark ? "#FFFFFF" : AppPalette.text
                    font.pixelSize: Tokens.fontLg
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                KCombo {
                    id: sourceCombo
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    enabled: page.liveDescriptors.length > 0
                    model: page.selectorLabels
                    toolTipText: qsTr("Video source for this pane")
                    onActivated: function(index) { page.pickSource(index) }
                }
            }
        }

        Binding {
            target: sourceCombo
            property: "currentIndex"
            value: page.selectorIndex
        }

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

}
