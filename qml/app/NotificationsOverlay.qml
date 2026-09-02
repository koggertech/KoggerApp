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
                                    tag: tag || "", actionPath: actionPath || "", closing: false })
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
            readonly property bool hasAction: model.actionPath !== undefined && model.actionPath.length > 0
            readonly property bool autoDismiss: !isWarning || root.hideImportant
            readonly property bool showClose: isWarning && !root.hideImportant
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
            width: Math.min(root.maxCardWidth,
                            Tokens.spaceLg + iconBadge.width + Tokens.spaceMd + messageText.implicitWidth
                            + Tokens.spaceMd + closeButton.width + Tokens.spaceLg)
            height: Math.max(Tokens.controlHLg, messageText.height + 2 * Tokens.spaceLg)
            radius: Tokens.radiusLg
            color: AppPalette.card
            border.width: 1
            border.color: isWarning ? AppPalette.dangerBorder : AppPalette.border

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
                width: Math.round(20 * AppPalette.scale)
                height: width
                radius: width / 2
                // info → green "i", warning → yellow "!"
                color: card.isWarning ? "#EAB308" : "#22C55E"

                Text {
                    anchors.centerIn: parent
                    text: card.isWarning ? "!" : "i"
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
                text: model.text
                color: AppPalette.text
                font.pixelSize: Tokens.fontBase
                wrapMode: Text.Wrap
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
