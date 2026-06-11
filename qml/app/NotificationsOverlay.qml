import QtQuick 2.15
import kqml_types 1.0

Item {
    id: root

    anchors.fill: parent
    z: ZOrder.notificationsOverlay

    readonly property int maxVisible: 5
    readonly property int infoLifetimeMs: 3000
    readonly property int exitMs: 220
    readonly property real maxCardWidth: Math.min(480 * AppPalette.scale, width - 2 * Tokens.spaceXl)
    property int nextNotificationId: 0

    signal tagDismissRequested(string tag)

    function push(kind, text, tag) {
        if (notificationsModel.count >= maxVisible)
            evictOldestInfo()
        notificationsModel.append({ notificationId: nextNotificationId++, kind: kind, text: text, tag: tag || "" })
    }

    function evictOldestInfo() {
        for (var i = 0; i < notificationsModel.count; ++i) {
            if (notificationsModel.get(i).kind === 0) {
                notificationsModel.remove(i)
                return
            }
        }
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
        function onMessageRequested(kind, text, tag) { root.push(kind, text, tag) }
        function onDismissRequested(tag) { root.tagDismissRequested(tag) }
    }

    ListModel { id: notificationsModel }

    Column {
        anchors.top: parent.top
        anchors.topMargin: Tokens.spaceLg
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: Tokens.spaceMd

        move: Transition {
            NumberAnimation { properties: "y"; duration: 180; easing.type: Easing.OutCubic }
        }
        add: Transition {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 180; easing.type: Easing.OutCubic }
            NumberAnimation { property: "y"; duration: 180; easing.type: Easing.OutCubic }
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
            property bool closing: false

            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(root.maxCardWidth,
                            accentBar.width + Tokens.spaceLg + messageText.implicitWidth
                            + Tokens.spaceMd + closeButton.width + 2 * Tokens.spaceLg)
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
                exitAnim.start()
            }

            Timer {
                id: lifeTimer
                interval: root.infoLifetimeMs
                running: !card.isWarning
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
                NumberAnimation {
                    target: card
                    property: "opacity"
                    to: 0
                    duration: root.exitMs
                    easing.type: Easing.OutCubic
                }
                ScriptAction { script: root.removeById(model.notificationId) }
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.AllButtons
                cursorShape: card.isWarning ? Qt.ArrowCursor : Qt.PointingHandCursor
                onClicked: function(m) {
                    if (!card.isWarning)
                        card.dismiss()
                    m.accepted = true
                }
                onWheel: function(w) { w.accepted = true }
            }

            Rectangle {
                id: accentBar
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.margins: Math.round(4 * AppPalette.scale)
                width: Math.round(3 * AppPalette.scale)
                radius: width / 2
                color: card.isWarning ? AppPalette.dangerText : AppPalette.accentBar
            }

            Text {
                id: messageText
                anchors.left: accentBar.right
                anchors.leftMargin: Tokens.spaceLg
                anchors.right: closeButton.left
                anchors.rightMargin: card.isWarning ? Tokens.spaceMd : Tokens.spaceLg
                anchors.verticalCenter: parent.verticalCenter
                text: model.text
                color: AppPalette.text
                font.pixelSize: Tokens.fontBase
                wrapMode: Text.Wrap
            }

            KCircleIconButton {
                id: closeButton
                anchors.right: parent.right
                anchors.rightMargin: card.isWarning ? Tokens.spaceSm : 0
                anchors.verticalCenter: parent.verticalCenter
                visible: card.isWarning
                width: card.isWarning ? Tokens.controlHSm : 0
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
