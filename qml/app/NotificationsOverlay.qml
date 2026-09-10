import QtQuick 2.15
import kqml_types 1.0

Item {
    id: root

    anchors.fill: parent
    z: ZOrder.notificationsOverlay

    readonly property int maxVisible: 5
    readonly property int infoLifetimeMs: 3000
    readonly property real maxCardWidth: Math.min(480 * AppPalette.scale, width - 2 * Tokens.spaceXl)
    property int nextNotificationId: 0

    property bool hideImportant: false

    signal tagDismissRequested(string tag)

    readonly property int burstSlack: 2

    function push(kind, text, tag, actionPath) {
        if (notificationsModel.count >= maxVisible)
            evictOldestInfo()
        while (notificationsModel.count >= maxVisible + burstSlack && dropOldestInfo())
            ;
        notificationsModel.append({ notificationId: nextNotificationId++, kind: kind, text: text,
                                    tag: tag || "", actionPath: actionPath || "", closing: false,
                                    percent: -2 })
    }

    function pushProgress(text, tag, percent) {
        for (var i = 0; i < notificationsModel.count; ++i) {
            if (notificationsModel.get(i).kind === 2 && notificationsModel.get(i).tag === tag) {
                notificationsModel.setProperty(i, "text", text)
                notificationsModel.setProperty(i, "percent", percent)
                return
            }
        }
        if (notificationsModel.count >= maxVisible)
            evictOldestInfo()
        notificationsModel.insert(0, { notificationId: nextNotificationId++, kind: 2, text: text,
                                       tag: tag, actionPath: "", closing: false, percent: percent })
    }

    function evictOldestInfo() {
        for (var i = 0; i < notificationsModel.count; ++i) {
            var item = notificationsModel.get(i)
            if (item.kind === 0 && !item.closing) {
                notificationsModel.setProperty(i, "closing", true)
                return
            }
        }
        dropOldestInfo()
    }

    function dropOldestInfo() {
        for (var i = 0; i < notificationsModel.count; ++i) {
            if (notificationsModel.get(i).kind === 0) {
                notificationsModel.remove(i)
                return true
            }
        }
        return false
    }

    function reveal(path) {
        if (path.length > 0 && typeof core !== "undefined" && core)
            core.revealInFolder(path)
    }

    function removeById(id) {
        for (var i = 0; i < notificationsModel.count; ++i) {
            if (notificationsModel.get(i).notificationId === id) {
                notificationsModel.remove(i)
                return
            }
        }
    }

    Connections {
        target: typeof notifications !== "undefined" ? notifications : null
        ignoreUnknownSignals: true
        function onMessageRequested(kind, text, tag, actionPath) { root.push(kind, text, tag, actionPath) }
        function onProgressRequested(text, tag, percent) { root.pushProgress(text, tag, percent) }
        function onDismissRequested(tag) { root.tagDismissRequested(tag) }
    }

    ListModel { id: notificationsModel }

    Column {
        anchors.top: parent.top
        anchors.topMargin: Tokens.spaceLg
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: Tokens.spaceMd

        move: Transition {
            NumberAnimation { properties: "y"; duration: Anim.toastReflowMs; easing.type: Anim.toastReflowEasing }
        }

        Repeater {
            model: notificationsModel
            delegate: notificationCardComponent
        }
    }

    Component {
        id: notificationCardComponent

        Rectangle {
            id: card

            readonly property bool isWarning: model.kind === 1
            readonly property bool isProgress: model.kind === 2
            readonly property bool hasAction: model.actionPath !== undefined && model.actionPath.length > 0
            readonly property bool autoDismiss: !isProgress && (!isWarning || root.hideImportant)
            readonly property bool showClose: !isProgress && isWarning && !root.hideImportant
            readonly property real headerOffset: isProgress ? -(progressTrack.height + Tokens.spaceMd) / 2 : 0
            property bool closing: false

            readonly property bool modelClosing: model.closing === true
            onModelClosingChanged: if (modelClosing) dismiss()

            opacity: 0
            clip: true
            transformOrigin: Item.Top

            transform: [
                Scale {
                    id: pop
                    origin.x: card.width / 2
                    origin.y: 0
                    xScale: Anim.toastEnterScale
                    yScale: Anim.toastEnterScale
                },
                Translate {
                    id: slide
                    y: -Math.round(Anim.toastSlidePx * AppPalette.scale)
                }
            ]

            scale: pressArea.pressed ? Anim.dipScale(width)
                 : (pressArea.containsMouse ? Anim.liftScale(width) : 1.0)
            Behavior on scale {
                NumberAnimation { duration: Anim.controlMs; easing.type: Anim.controlEasing }
            }

            Component.onCompleted: enterAnim.start()

            ParallelAnimation {
                id: enterAnim

                NumberAnimation {
                    target: card; property: "opacity"; from: 0.0; to: 1.0
                    duration: Anim.fadeMs; easing.type: Anim.fadeEasing
                }
                NumberAnimation {
                    target: pop; properties: "xScale,yScale"
                    from: Anim.toastEnterScale; to: 1.0
                    duration: Anim.toastEnterMs
                    easing.type: Anim.toastEnterEasing
                    easing.overshoot: Anim.toastOvershoot
                }
                NumberAnimation {
                    target: slide; property: "y"
                    from: -Math.round(Anim.toastSlidePx * AppPalette.scale); to: 0
                    duration: Anim.toastEnterMs
                    easing.type: Anim.toastEnterEasing
                    easing.overshoot: Anim.toastOvershoot
                }
            }

            anchors.horizontalCenter: parent.horizontalCenter
            width: card.isProgress
                   ? root.maxCardWidth
                   : Math.min(root.maxCardWidth,
                              Tokens.spaceLg + iconBadge.width + Tokens.spaceMd + messageText.implicitWidth
                              + Tokens.spaceMd + closeButton.width + Tokens.spaceLg)
            height: Math.max(Tokens.controlHLg, messageText.height + 2 * Tokens.spaceLg)
                    + (card.isProgress ? progressTrack.height + Tokens.spaceMd : 0)
            radius: Tokens.radiusLg
            color: AppPalette.card
            border.width: 1
            border.color: isProgress ? AppPalette.accentBorder
                                     : (isWarning ? AppPalette.dangerBorder : AppPalette.border)

            function dismiss() {
                if (closing)
                    return
                closing = true
                lifeTimer.stop()
                enterAnim.stop()
                exitAnim.start()
            }

            Timer {
                id: lifeTimer
                interval: root.infoLifetimeMs
                running: card.autoDismiss
                onTriggered: card.dismiss()
            }

            Connections {
                target: root
                function onTagDismissRequested(tag) {
                    if (tag.length && model.tag === tag)
                        card.dismiss()
                }
            }

            SequentialAnimation {
                id: exitAnim

                ParallelAnimation {
                    NumberAnimation {
                        target: card; property: "opacity"; to: 0
                        duration: Anim.toastExitMs; easing.type: Anim.toastExitEasing
                    }
                    NumberAnimation {
                        target: pop; properties: "xScale,yScale"; to: Anim.toastExitScale
                        duration: Anim.toastExitMs; easing.type: Anim.toastExitEasing
                    }
                    NumberAnimation {
                        target: card; property: "height"; to: 0
                        duration: Anim.toastExitMs; easing.type: Anim.toastExitEasing
                    }
                }
                ScriptAction { script: root.removeById(model.notificationId) }
            }

            MouseArea {
                id: pressArea
                anchors.fill: parent
                enabled: !card.closing
                hoverEnabled: true
                acceptedButtons: Qt.AllButtons
                cursorShape: card.autoDismiss ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: function(m) {
                    m.accepted = true
                    var path = card.hasAction ? String(model.actionPath) : ""
                    if (card.autoDismiss)
                        card.dismiss()
                    if (path.length > 0)
                        Qt.callLater(root.reveal, path)
                }
                onWheel: function(w) { w.accepted = true }
            }

            Rectangle {
                id: iconBadge
                anchors.left: parent.left
                anchors.leftMargin: Tokens.spaceLg
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: card.headerOffset
                width: Math.round(20 * AppPalette.scale)
                height: width
                radius: width / 2
                // info → green "i", warning → yellow "!", progress → accent "↑"
                color: card.isProgress ? AppPalette.accentBar : (card.isWarning ? "#EAB308" : "#22C55E")

                Text {
                    anchors.centerIn: parent
                    text: card.isProgress ? "↑" : (card.isWarning ? "!" : "i")
                    color: "#10171F"
                    font.pixelSize: Math.round(13 * AppPalette.scale)
                    font.bold: true
                }
            }

            Text {
                id: messageText
                anchors.left: iconBadge.right
                anchors.leftMargin: Tokens.spaceMd
                anchors.right: closeButton.left
                anchors.rightMargin: card.showClose ? Tokens.spaceMd : Tokens.spaceLg
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: card.headerOffset
                text: model.text
                color: AppPalette.text
                font.pixelSize: Tokens.fontBase
                wrapMode: Text.Wrap
            }

            Rectangle {
                id: progressTrack
                visible: card.isProgress
                anchors.left: iconBadge.left
                anchors.right: parent.right
                anchors.rightMargin: Tokens.spaceLg
                anchors.bottom: parent.bottom
                anchors.bottomMargin: Tokens.spaceLg
                height: card.isProgress ? Math.round(4 * AppPalette.scale) : 0
                radius: height / 2
                color: AppPalette.trackOff
                clip: true

                readonly property bool indeterminate: model.percent < 0

                Rectangle {
                    visible: !progressTrack.indeterminate
                    width: progressTrack.width * Math.max(0, Math.min(100, model.percent)) / 100
                    height: parent.height
                    radius: parent.radius
                    color: AppPalette.accentBar
                    Behavior on width { NumberAnimation { duration: Anim.controlMs; easing.type: Anim.controlEasing } }
                }

                Rectangle {
                    id: busyChip
                    visible: progressTrack.indeterminate
                    width: Math.round(progressTrack.width * 0.3)
                    height: parent.height
                    radius: parent.radius
                    color: AppPalette.accentBar

                    SequentialAnimation on x {
                        running: busyChip.visible
                        loops: Animation.Infinite
                        NumberAnimation { from: 0; to: progressTrack.width - busyChip.width
                                          duration: 900; easing.type: Easing.InOutQuad }
                        NumberAnimation { from: progressTrack.width - busyChip.width; to: 0
                                          duration: 900; easing.type: Easing.InOutQuad }
                    }
                }
            }

            KCircleIconButton {
                id: closeButton
                anchors.right: parent.right
                anchors.rightMargin: card.showClose ? Tokens.spaceSm : 0
                anchors.verticalCenter: parent.verticalCenter
                visible: card.showClose
                width: card.showClose ? Tokens.controlHSm : 0
                height: Tokens.controlHSm
                glyph: "×"
                glyphPixelSize: Math.round(12 * AppPalette.scale)
                fillColor: "transparent"
                fillHoverColor: AppPalette.cardHover
                borderColor: "transparent"
                onClicked: card.dismiss()
            }
        }
    }
}
