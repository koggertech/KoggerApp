import QtQuick 2.15
import QtMultimedia
import kqml_types 1.0

Item {
    id: root

    property var store: null
    property string contentId: ""

    readonly property real chromeIdleOpacity: 0.45
    readonly property bool selectorOpen: sourceCombo.popup.visible

    readonly property var descriptors: (typeof videoStreams !== "undefined" && videoStreams)
                                        ? (videoStreams.streams || [])
                                        : []

    readonly property var liveDescriptors: {
        var out = []
        for (var i = 0; i < descriptors.length; ++i) {
            if (descriptors[i].open)
                out.push(descriptors[i])
        }
        return out
    }

    readonly property string sourceUuid: {
        if (!store || !contentId.length)
            return ""
        var map = store.videoSourceByContent
        var uuid = map ? map[contentId] : ""
        return uuid ? String(uuid) : ""
    }

    readonly property var sourceDescriptor: {
        for (var i = 0; i < liveDescriptors.length; ++i) {
            if (String(liveDescriptors[i].uuid) === root.sourceUuid)
                return liveDescriptors[i]
        }
        return null
    }

    readonly property var stream: (root.sourceDescriptor && typeof videoStreams !== "undefined" && videoStreams)
                                   ? videoStreams.streamFor(root.sourceUuid)
                                   : null

    readonly property bool hasFrame: stream ? stream.hasFrame : false
    readonly property int sourceWidth: stream ? stream.sourceWidth : 0
    readonly property int sourceHeight: stream ? stream.sourceHeight : 0
    readonly property string statusText: stream ? stream.statusText : ""

    readonly property var selectableDescriptors: {
        var out = [{ uuid: "", label: qsTr("Not selected") }]
        for (var i = 0; i < liveDescriptors.length; ++i)
            out.push(liveDescriptors[i])
        return out
    }

    readonly property var selectorLabels: {
        var out = []
        for (var i = 0; i < selectableDescriptors.length; ++i)
            out.push(String(selectableDescriptors[i].label))
        return out
    }

    readonly property int selectorIndex: {
        for (var i = 0; i < selectableDescriptors.length; ++i) {
            if (String(selectableDescriptors[i].uuid) === root.sourceUuid)
                return i
        }
        return 0
    }

    property string boundUuid: ""

    function releaseSink() {
        if (!boundUuid.length)
            return
        if (typeof videoStreams !== "undefined" && videoStreams) {
            var previous = videoStreams.streamFor(boundUuid)
            if (previous)
                previous.removeSink(videoOutputItem.videoSink)
        }
        boundUuid = ""
    }

    function rebindSink() {
        var wanted = root.stream ? root.sourceUuid : ""
        if (boundUuid === wanted)
            return

        releaseSink()

        if (wanted.length && root.stream) {
            root.stream.addSink(videoOutputItem.videoSink)
            boundUuid = wanted
        }
    }

    onStreamChanged: rebindSink()
    onSourceUuidChanged: rebindSink()
    Component.onCompleted: rebindSink()
    Component.onDestruction: releaseSink()

    Rectangle {
        anchors.fill: parent
        color: root.hasFrame ? "black" : AppPalette.headerBg
    }

    Item {
        id: placeholder
        anchors.fill: parent
        visible: !root.hasFrame

        Column {
            anchors.centerIn: parent
            spacing: Math.round(12 * AppPalette.scale)

            Image {
                id: logo
                anchors.horizontalCenter: parent.horizontalCenter
                source: "qrc:/icons/app/kogger_app.png"
                fillMode: Image.PreserveAspectFit
                smooth: true
                mipmap: true
                opacity: 0.75

                readonly property int _side: Math.round(Math.min(root.width, root.height) * 0.32)
                width: Math.max(Math.round(48 * AppPalette.scale),
                                Math.min(_side, Math.round(220 * AppPalette.scale)))
                height: width
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                width: Math.min(implicitWidth, root.width - Tokens.spaceLg * 2)
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideMiddle
                text: root.sourceDescriptor === null ? qsTr("No video source") : root.statusText
                color: AppPalette.textMuted
                font.pixelSize: Tokens.fontBase
                visible: root.height > Math.round(120 * AppPalette.scale)
            }
        }
    }

    VideoOutput {
        id: videoOutputItem
        anchors.fill: parent
        visible: root.hasFrame
        fillMode: {
            var mode = root.store ? root.store.videoFillForContent(root.contentId) : 0
            return mode === 1 ? VideoOutput.PreserveAspectCrop
                 : mode === 2 ? VideoOutput.Stretch
                              : VideoOutput.PreserveAspectFit
        }
    }

    Row {
        id: overlayControls
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: Tokens.spaceSm
        spacing: Tokens.spaceXs
        visible: root.contentId.length > 0
        opacity: (overlayHover.hovered || sourceCombo.popup.visible)
                 ? 1.0
                 : root.chromeIdleOpacity
        Behavior on opacity { NumberAnimation { duration: Anim.fadeMs } }

        HoverHandler { id: overlayHover }

        KCircleIconButton {
            width: Tokens.controlHMd
            height: Tokens.controlHMd
            iconSource: "qrc:/icons/ui/settings.svg"
            iconTintColor: AppPalette.text
            toolTipText: qsTr("Video window settings")
            onClicked: if (root.store) root.store.openVideoPaneSettings(root.contentId)
        }

        Item {
            id: sourceSelector

            readonly property int available: root.width - Tokens.spaceSm * 2
                                             - Tokens.controlHMd - Tokens.spaceXs
            width: Math.max(0, Math.min(available, sourceCombo.contentWidth))
            height: Tokens.controlHMd
            visible: root.liveDescriptors.length > 0

            KCombo {
                id: sourceCombo
                anchors.fill: parent
                fontPixelSize: Tokens.fontSm
                bold: false
                focusHighlight: false
                toolTipText: qsTr("Video source for this pane")
                model: root.selectorLabels
                onActivated: function(index) {
                    if (!root.store || index < 0 || index >= root.selectableDescriptors.length)
                        return
                    var picked = root.selectableDescriptors[index]
                    root.store.setVideoSourceForContent(root.contentId, String(picked.uuid))
                }
            }

            Binding {
                target: sourceCombo
                property: "currentIndex"
                value: root.selectorIndex
            }
        }
    }

    Rectangle {
        id: resolutionBadge
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: Tokens.spaceSm
        visible: root.hasFrame
                 && root.width > Math.round(180 * AppPalette.scale)
                 && (root.store ? root.store.videoResolutionVisibleForContent(root.contentId) : true)
        radius: Tokens.radiusMd
        color: AppPalette.bg
        border.width: Tokens.cardBorderWidth
        border.color: AppPalette.border
        width: resolutionText.implicitWidth + Tokens.spaceSm * 2
        height: Tokens.controlHMd

        property bool revealed: false
        opacity: (badgeHover.hovered || revealed) ? 1.0 : root.chromeIdleOpacity
        Behavior on opacity { NumberAnimation { duration: Anim.fadeMs } }

        HoverHandler { id: badgeHover }
        TapHandler { onTapped: resolutionBadge.revealed = !resolutionBadge.revealed }

        Text {
            id: resolutionText
            anchors.centerIn: parent
            text: root.sourceWidth + "×" + root.sourceHeight
            color: AppPalette.text
            font.pixelSize: Tokens.fontSm
        }
    }
}
