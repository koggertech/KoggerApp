import QtQuick 2.15
import QtMultimedia

Item {
    id: root

    property var workspaceItem: null
    readonly property var store: workspaceItem ? workspaceItem.store : null

    readonly property int sourceWidth: videoStream.sourceWidth
    readonly property int sourceHeight: videoStream.sourceHeight
    readonly property bool hasFrame: videoStream.hasFrame

    Connections {
        target: root.store
        ignoreUnknownSignals: true

        function onVideoActiveUrlChanged() {
            var url = root.store ? root.store.videoActiveUrl : ""
            if (url.length) {
                root.workspaceItem.videoLog("open " + url)
                videoStream.start(url)
            }
            else {
                root.workspaceItem.videoLog("closed")
                var closed = root.store ? root.store.videoUrl : ""
                videoStream.stop()
                if (closed.length)
                    notifications.info(qsTr("Video stream closed: %1").arg(closed))
            }
        }
    }

    onHasFrameChanged: {
        if (hasFrame && root.store)
            notifications.info(qsTr("Video stream opened: %1").arg(root.store.videoActiveUrl))
    }

    Connections {
        target: videoStream

        function onRetriesExhausted() {
            if (root.store)
                root.store.stopVideo()
        }

        function onStatusTextChanged() {
            if (root.store)
                root.store.videoStatusText = videoStream.statusText
            if (videoStream.statusText.length)
                root.workspaceItem.videoLog("status: " + videoStream.statusText)
        }
    }

    VideoOutput {
        id: videoOutputItem
        parent: root.workspaceItem && root.workspaceItem.activeVideoHostItem ? root.workspaceItem.activeVideoHostItem : root
        anchors.fill: parent
        visible: root.workspaceItem !== null
                 && root.workspaceItem.activeVideoHostItem !== null
                 && root.hasFrame
        fillMode: {
            var m = root.store ? root.store.videoFillMode : 0
            return m === 1 ? VideoOutput.PreserveAspectCrop
                 : m === 2 ? VideoOutput.Stretch
                           : VideoOutput.PreserveAspectFit
        }

        Binding {
            target: videoStream
            property: "videoSink"
            value: videoOutputItem.videoSink
        }
    }

    Component.onDestruction: videoStream.stop()
}
