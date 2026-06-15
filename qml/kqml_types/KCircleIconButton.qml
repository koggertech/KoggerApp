import QtQuick 2.15
import QtQuick.Window 2.15
import Qt5Compat.GraphicalEffects
import kqml_types 1.0

Item {
    id: root

    property string glyph: ""
    property url iconSource: ""
    property int iconPixelSize: Math.round(Math.min(width, height) * 0.56)
    property bool showGlyphWithIcon: false
    property int glyphPixelSize: 18
    property color glyphColor: AppPalette.text
    property color iconTintColor: "transparent"
    property color fillColor: "#1E293BCC"
    property color fillHoverColor: "#0F172ACC"
    property color fillPressedColor: AppPalette.bgDeep
    property color borderColor: AppPalette.border
    property color borderHoverColor: AppPalette.borderHover
    property real borderWidth: 1
    property bool enabled: true
    activeFocusOnTab: enabled
    Keys.onReturnPressed: if (root.enabled) root.clicked()
    Keys.onEnterPressed:  if (root.enabled) root.clicked()
    Keys.onSpacePressed:  if (root.enabled) root.clicked()
    property bool rounded: true
    property real cornerRadius: rounded ? Math.min(width, height) / 2 : 10
    property int cursorShape: Qt.PointingHandCursor
    property int focusPolicy: Qt.NoFocus
    property real padding: 0
    property string toolTipText: ""
    property bool autoToolTip: true
    property real hoverWhiteness: 0.12
    // External "look here" pulse — bumped via flashToken when highlighted.
    property bool highlighted: false
    property int flashToken: 0
    property color highlightBorderColor: AppPalette.accentBorder
    readonly property bool hovered: hitArea.containsMouse
    readonly property bool pressed: hitArea.pressed

    property bool _tipSuppressed: false
    onPressedChanged: if (pressed) { _tipSuppressed = true; focusRing.suppress() }
    onHoveredChanged: if (!hovered) _tipSuppressed = false
    readonly property real backgroundScale: !root.enabled ? 1.0 : (root.pressed ? 0.97 : (root.hovered ? 1.035 : 1.0))
    readonly property bool hasIcon: {
        var s = iconSource ? iconSource.toString() : ""
        return s !== "" && s.charAt(s.length - 1) !== "/"
    }
    readonly property string resolvedToolTipText: {
        if (toolTipText !== "")
            return toolTipText
        if (!autoToolTip)
            return ""
        if (glyph === "\u2715" || glyph === "\u00D7")
            return qsTr("Close")
        if (glyph === "\u2261")
            return qsTr("Menu")
        if (glyph.indexOf("\u2699") !== -1)
            return qsTr("Settings")
        if (glyph === "P")
            return qsTr("Assign popup")
        if (glyph === "2D" || glyph === "3D")
            return qsTr("Pane mode")
        if (glyph === "MOVE")
            return qsTr("Move pane")
        return ""
    }

    signal clicked()
    signal pressStarted(real mouseX, real mouseY)
    signal pointerMoved(real mouseX, real mouseY)
    signal pressEnded()
    signal pressCanceled()

    implicitWidth: 36
    implicitHeight: 36

    Rectangle {
        id: backgroundRect
        anchors.fill: parent
        radius: root.cornerRadius
        scale: root.backgroundScale
        color: !root.enabled
               ? "#0F172A55"
               : (root.pressed
                  ? root.fillPressedColor
                  : (root.hovered ? root.fillHoverColor : root.fillColor))
        border.width: root.borderWidth
        border.color: !root.enabled
                      ? "#47556966"
                      : (root.hovered ? root.borderHoverColor : root.borderColor)

        Behavior on color {
            ColorAnimation {
                duration: 110
                easing.type: Easing.OutCubic
            }
        }

        Behavior on border.color {
            ColorAnimation {
                duration: 110
                easing.type: Easing.OutCubic
            }
        }

        Behavior on scale {
            NumberAnimation {
                duration: 120
                easing.type: Easing.OutCubic
            }
        }
    }

    Rectangle {
        anchors.fill: backgroundRect
        radius: root.cornerRadius
        scale: root.backgroundScale
        color: "#FFFFFF"
        opacity: !root.enabled ? 0.0 : (root.pressed ? 0.02 : (root.hovered ? root.hoverWhiteness : 0.0))

        Behavior on opacity {
            NumberAnimation {
                duration: 110
                easing.type: Easing.OutCubic
            }
        }

        Behavior on scale {
            NumberAnimation {
                duration: 120
                easing.type: Easing.OutCubic
            }
        }
    }

    KFocusRing {
        id: focusRing
        target: backgroundRect
        focusItem: root
        radius: root.cornerRadius
    }

    Item {
        id: iconWrap
        anchors.centerIn: parent
        visible: root.hasIcon
        width: root.iconPixelSize
        height: root.iconPixelSize

        readonly property bool tintActive: root.iconTintColor.a > 0

        Image {
            id: iconImage
            anchors.fill: parent
            source: root.iconSource
            sourceSize.width: Math.max(1, Math.round(width * Screen.devicePixelRatio))
            sourceSize.height: Math.max(1, Math.round(height * Screen.devicePixelRatio))
            fillMode: Image.PreserveAspectFit
            cache: true
            asynchronous: false
            smooth: true
            mipmap: false
            opacity: root.enabled ? 1.0 : 0.7
            visible: !iconWrap.tintActive
            layer.enabled: iconWrap.tintActive
        }

        ColorOverlay {
            anchors.fill: iconImage
            source: iconImage
            visible: iconWrap.tintActive
            color: root.iconTintColor
            cached: true
        }
    }

    // "×" / "✕" glyphs are drawn geometrically — Text's bbox center doesn't
    // match the visual cross center in most fonts. Two rotated bars give a
    // pixel-perfect, perfectly-centered close icon.
    readonly property bool _isCrossGlyph: root.glyph === "×" || root.glyph === "✕"

    Item {
        anchors.centerIn: parent
        visible: (!root.hasIcon || root.showGlyphWithIcon) && root._isCrossGlyph
        width: root.glyphPixelSize
        height: root.glyphPixelSize

        readonly property color _stroke: root.enabled
                                         ? (root.hovered ? Qt.lighter(root.glyphColor, 1.12) : root.glyphColor)
                                         : AppPalette.textMuted
        readonly property int _thickness: Math.max(2, Math.round(root.glyphPixelSize * 0.15))

        Rectangle {
            anchors.centerIn: parent
            width: parent.width
            height: parent._thickness
            radius: height / 2
            color: parent._stroke
            rotation: 45
        }
        Rectangle {
            anchors.centerIn: parent
            width: parent.width
            height: parent._thickness
            radius: height / 2
            color: parent._stroke
            rotation: -45
        }

        Behavior on opacity {
            NumberAnimation { duration: 110; easing.type: Easing.OutCubic }
        }
    }

    Text {
        anchors.centerIn: parent
        visible: (!root.hasIcon || root.showGlyphWithIcon) && !root._isCrossGlyph
        text: root.glyph
        color: root.enabled
               ? (root.hovered ? Qt.lighter(root.glyphColor, 1.12) : root.glyphColor)
               : AppPalette.textMuted
        font.pixelSize: root.glyphPixelSize
        font.bold: true

        Behavior on color {
            ColorAnimation {
                duration: 110
                easing.type: Easing.OutCubic
            }
        }
    }

    // Pulse overlay — flashes when flashToken changes while highlighted.
    Rectangle {
        id: highlightOverlay
        anchors.fill: parent
        radius: root.cornerRadius
        color: "transparent"
        border.width: Math.max(2, Math.round(2 * AppPalette.scale))
        border.color: root.highlightBorderColor
        opacity: 0
        visible: root.highlighted
        z: 10
    }

    SequentialAnimation {
        id: highlightPulse
        running: false
        NumberAnimation { target: highlightOverlay; property: "opacity"; to: 0.95; duration: 90;  easing.type: Easing.OutCubic }
        NumberAnimation { target: highlightOverlay; property: "opacity"; to: 0.30; duration: 180; easing.type: Easing.OutCubic }
        NumberAnimation { target: highlightOverlay; property: "opacity"; to: 0.0;  duration: 280; easing.type: Easing.OutCubic }
    }

    onFlashTokenChanged: {
        if (highlighted)
            highlightPulse.restart()
    }

    onHighlightedChanged: {
        if (!highlighted)
            highlightOverlay.opacity = 0.0
    }

    // Trigger initial pulse when the delegate spawns already highlighted —
    // onFlashTokenChanged only fires on changes after construction.
    Component.onCompleted: {
        if (highlighted)
            highlightPulse.restart()
    }

    MouseArea {
        id: hitArea
        anchors.fill: parent
        enabled: root.enabled
        hoverEnabled: true
        cursorShape: root.enabled ? root.cursorShape : Qt.ArrowCursor
        onPressed: function(mouse) { root.pressStarted(mouse.x, mouse.y) }
        onPositionChanged: function(mouse) { root.pointerMoved(mouse.x, mouse.y) }
        onReleased: root.pressEnded()
        onCanceled: root.pressCanceled()
        onClicked: { root.forceActiveFocus(); root.clicked() }
    }

    KToolTip {
        text: root.resolvedToolTipText
        targetItem: root
        shown: root.hovered && root.enabled && !root._tipSuppressed
    }
}
