import QtQuick 2.15
import kqml_types 1.0

Item {
    id: root

    property var workspaceRoot: null
    property int leafId: -1
    property var paneData: ({
        title: "",
        color: "transparent",
        mode: "Video"
    })
    property bool rotateEnabled: false
    property int lastRegisteredLeafId: -1

    readonly property var store: workspaceRoot ? workspaceRoot.store : null
    readonly property string activeUrl: store ? store.videoActiveUrl : ""
    readonly property string statusText: store ? store.videoStatusText : ""
    readonly property bool hasFrame: workspaceRoot ? workspaceRoot.videoHasFrame : false
    readonly property int sourceWidth: workspaceRoot ? workspaceRoot.videoSourceWidth : 0
    readonly property int sourceHeight: workspaceRoot ? workspaceRoot.videoSourceHeight : 0

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
                text: root.activeUrl.length === 0 ? qsTr("No video source") : root.statusText
                color: AppPalette.textMuted
                font.pixelSize: Tokens.fontBase
                visible: root.height > Math.round(120 * AppPalette.scale)
            }
        }
    }

    Item {
        id: hostSurface
        anchors.fill: parent
        clip: true
    }

    Rectangle {
        id: resolutionBadge
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: Tokens.spaceSm
        visible: root.hasFrame && root.width > Math.round(180 * AppPalette.scale)
        radius: Tokens.radiusSm
        color: "#00000080"
        width: resolutionText.implicitWidth + Tokens.spaceMd * 2
        height: resolutionText.implicitHeight + Tokens.spaceXs * 2

        Text {
            id: resolutionText
            anchors.centerIn: parent
            text: root.sourceWidth + "×" + root.sourceHeight
            color: "#FFFFFF"
            font.pixelSize: Tokens.fontSm
        }
    }

    KButton {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Tokens.spaceSm
        visible: root.activeUrl.length > 0
        height: Tokens.controlHMd
        text: qsTr("Stop")
        onClicked: if (root.store) root.store.stopVideo()
    }

    function hostMode() {
        return "Video"
    }

    function syncHostRegistration() {
        if (workspaceRoot && typeof workspaceRoot.unregisterPaneHost === "function" && lastRegisteredLeafId !== -1 && lastRegisteredLeafId !== leafId) {
            workspaceRoot.unregisterPaneHost(lastRegisteredLeafId, hostSurface)
            lastRegisteredLeafId = -1
        }

        if (!workspaceRoot || typeof workspaceRoot.registerPaneHost !== "function" || leafId < 0) {
            return
        }

        workspaceRoot.registerPaneHost(leafId, hostSurface, hostMode())
        lastRegisteredLeafId = leafId
    }

    Component.onCompleted: syncHostRegistration()
    onWorkspaceRootChanged: syncHostRegistration()
    onLeafIdChanged: syncHostRegistration()

    Component.onDestruction: {
        if (workspaceRoot && typeof workspaceRoot.unregisterPaneHost === "function" && lastRegisteredLeafId !== -1) {
            workspaceRoot.unregisterPaneHost(lastRegisteredLeafId, hostSurface)
        }
    }
}
