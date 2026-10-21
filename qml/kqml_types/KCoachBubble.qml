import QtQuick
import QtQuick.Layouts
import QtQuick.Shapes
import kqml_types 1.0

Item {
    id: root

    property string title: ""
    property string text: ""
    property url iconSource: ""
    property string counterText: ""
    property string hintText: ""
    property real maxWidth: Math.round(360 * AppPalette.scale)
    property string tailSide: ""
    property real tailOffset: width / 2
    property bool showBack: false
    property bool backEnabled: true
    property bool showSkip: false
    property bool showNext: false
    property string backLabel: qsTr("Back")
    property string skipLabel: qsTr("Skip")
    property string nextLabel: qsTr("Next")
    property bool showClose: false
    property string confirmTitle: qsTr("Leave the tutorial?")
    property string confirmText: qsTr("You can continue it later from Settings → Tutorials.")
    property bool confirming: false
    property bool shown: true

    signal backClicked()
    signal skipClicked()
    signal nextClicked()
    signal closeConfirmed()

    function nudge() {
        shakeAnim.restart()
    }

    property real _shake: 0
    transform: Translate { x: root._shake }

    SequentialAnimation {
        id: shakeAnim
        readonly property real amp: Math.round(6 * AppPalette.scale)
        NumberAnimation { target: root; property: "_shake"; to: shakeAnim.amp; duration: 45 }
        NumberAnimation { target: root; property: "_shake"; to: -shakeAnim.amp; duration: 90 }
        NumberAnimation { target: root; property: "_shake"; to: shakeAnim.amp / 2; duration: 70 }
        NumberAnimation { target: root; property: "_shake"; to: 0; duration: 60 }
    }

    readonly property real tailSize: Math.round(10 * AppPalette.scale)
    readonly property real _pad: Tokens.spaceXl
    readonly property real _border: Math.max(1, Math.round(AppPalette.scale))
    readonly property bool _hasIcon: root.iconSource.toString() !== ""
    readonly property bool _hasButtons: root.confirming || root.showSkip || root.showNext
    readonly property string _title: root.confirming ? root.confirmTitle : root.title
    readonly property string _text: root.confirming ? root.confirmText : root.text
    readonly property real _contentWidth: Math.max(titleRow.implicitWidth, bodyText.implicitWidth,
                                                   hintLabel.visible ? hintLabel.implicitWidth : 0,
                                                   buttonsRow.visible ? buttonsRow.implicitWidth : 0)

    width: Math.min(root.maxWidth, Math.max(Math.round(200 * AppPalette.scale), root._contentWidth + root._pad * 2))
    height: content.implicitHeight + root._pad * 2

    opacity: root.shown ? 1 : 0
    scale: root.shown ? 1 : 0.94
    visible: opacity > 0.01
    Behavior on opacity { NumberAnimation { duration: root.shown ? Anim.popupEnterMs : Anim.popupExitMs; easing.type: Anim.easingStd } }
    Behavior on scale { NumberAnimation { duration: root.shown ? Anim.popupEnterMs : Anim.popupExitMs; easing.type: Anim.easingBack } }

    Rectangle {
        anchors.fill: body
        anchors.topMargin: Math.round(3 * AppPalette.scale)
        anchors.bottomMargin: -Math.round(3 * AppPalette.scale)
        radius: body.radius
        color: AppPalette.shadowMid
    }

    Rectangle {
        id: body
        anchors.fill: parent
        radius: Tokens.radiusLg
        color: AppPalette.card
        border.width: root._border
        border.color: AppPalette.border
    }

    Item {
        id: tail
        visible: root.tailSide !== ""
        width: root.tailSize * 2
        height: root.tailSize + root._border
        x: root.tailSide === "top" || root.tailSide === "bottom"
           ? root.tailOffset - width / 2
           : (root.tailSide === "left" ? -root.tailSize - width / 2 + height / 2 : root.width + root.tailSize - width / 2 - height / 2)
        y: root.tailSide === "top" ? -root.tailSize
           : root.tailSide === "bottom" ? root.height - root._border
           : root.tailOffset - height / 2
        rotation: root.tailSide === "bottom" ? 180 : root.tailSide === "left" ? -90 : root.tailSide === "right" ? 90 : 0

        Shape {
            anchors.fill: parent
            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                fillColor: AppPalette.card
                strokeColor: "transparent"
                strokeWidth: -1
                startX: 0
                startY: tail.height
                PathLine { x: tail.width / 2; y: 0 }
                PathLine { x: tail.width; y: tail.height }
                PathLine { x: 0; y: tail.height }
            }

            ShapePath {
                fillColor: "transparent"
                strokeColor: AppPalette.border
                strokeWidth: root._border
                joinStyle: ShapePath.RoundJoin
                startX: 0
                startY: tail.height - root._border
                PathLine { x: tail.width / 2; y: 0 }
                PathLine { x: tail.width; y: tail.height - root._border }
            }
        }
    }

    ColumnLayout {
        id: content
        x: root._pad
        y: root._pad
        width: root.width - root._pad * 2
        spacing: Tokens.spaceMd

        RowLayout {
            id: titleRow
            Layout.fillWidth: true
            spacing: Tokens.spaceMd
            visible: root._title !== "" || root.counterText !== "" || root._hasIcon || root.showClose || root.showBack

            Image {
                visible: root._hasIcon && !root.confirming
                source: root.iconSource
                sourceSize.width: Tokens.iconLg
                sourceSize.height: Tokens.iconLg
                Layout.preferredWidth: Tokens.iconLg
                Layout.preferredHeight: Tokens.iconLg
                fillMode: Image.PreserveAspectFit
            }

            Text {
                Layout.fillWidth: true
                visible: root._title !== ""
                text: root._title
                color: AppPalette.textStrong
                font.pixelSize: Tokens.fontLg
                font.bold: true
                wrapMode: Text.Wrap
            }

            KCircleIconButton {
                visible: root.showBack && !root.confirming
                enabled: root.backEnabled
                Layout.preferredWidth: Tokens.controlHMd
                Layout.preferredHeight: Tokens.controlHMd
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                hitPadding: Tokens.spaceSm
                iconSource: "qrc:/icons/ui/chevron-right.svg"
                iconRotation: 180
                iconTintColor: AppPalette.textSecond
                fillColor: "transparent"
                fillHoverColor: AppPalette.cardHover
                borderWidth: 0
                toolTipText: root.backLabel
                onClicked: root.backClicked()
            }

            Text {
                visible: root.counterText !== "" && !root.confirming
                text: root.counterText
                color: AppPalette.textMuted
                font.pixelSize: Tokens.fontSm
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            }

            KCircleIconButton {
                visible: root.showClose && !root.confirming
                Layout.preferredWidth: Tokens.controlHMd
                Layout.preferredHeight: Tokens.controlHMd
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                hitPadding: Tokens.spaceSm
                iconSource: "qrc:/icons/ui/x.svg"
                iconTintColor: AppPalette.textSecond
                fillColor: "transparent"
                fillHoverColor: AppPalette.cardHover
                borderWidth: 0
                toolTipText: qsTr("Close tutorial")
                onClicked: root.confirming = true
            }
        }

        Text {
            id: bodyText
            Layout.fillWidth: true
            visible: root._text !== ""
            text: root._text
            color: AppPalette.text
            font.pixelSize: Tokens.fontBase
            lineHeight: 1.15
            wrapMode: Text.Wrap
        }

        Text {
            id: hintLabel
            Layout.fillWidth: true
            visible: root.hintText !== "" && !root.confirming
            text: root.hintText
            color: AppPalette.textMuted
            font.pixelSize: Tokens.fontSm
            font.italic: true
            wrapMode: Text.Wrap
        }

        RowLayout {
            id: buttonsRow
            Layout.fillWidth: true
            Layout.topMargin: Tokens.spaceXs
            visible: root._hasButtons
            spacing: Tokens.spaceMd

            KButton {
                visible: root.showSkip && !root.confirming
                text: root.skipLabel
                bold: false
                textColor: AppPalette.textSecond
                Layout.preferredHeight: Tokens.controlHLg
                focusPolicy: Qt.NoFocus
                onClicked: root.skipClicked()
            }

            Item { Layout.fillWidth: true }

            KButton {
                visible: root.confirming
                text: qsTr("Stay")
                normalBg: AppPalette.controlRaised
                Layout.preferredHeight: Tokens.controlHLg
                focusPolicy: Qt.NoFocus
                onClicked: root.confirming = false
            }

            KButton {
                visible: root.confirming
                text: qsTr("Leave")
                danger: true
                Layout.preferredHeight: Tokens.controlHLg
                focusPolicy: Qt.NoFocus
                onClicked: {
                    root.confirming = false
                    root.closeConfirmed()
                }
            }

            KButton {
                visible: root.showNext && !root.confirming
                text: root.nextLabel
                normalBg: AppPalette.accentBg
                hoverBg: AppPalette.accentBgHover
                normalBorder: AppPalette.accentBorder
                hoverBorder: AppPalette.accentBorder
                textColor: AppPalette.accentText
                Layout.preferredHeight: Tokens.controlHLg
                focusPolicy: Qt.NoFocus
                onClicked: root.nextClicked()
            }
        }
    }
}
