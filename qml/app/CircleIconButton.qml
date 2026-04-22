import QtQuick 2.15
import QtQuick.Window 2.15
import kqml_types 1.0

Item {
    id: root

    property string glyph: ""
    property url iconSource: ""
    property int iconPixelSize: Math.round(Math.min(width, height) * 0.56)
    property bool showGlyphWithIcon: false
    property int glyphPixelSize: 18
    property color glyphColor: AppPalette.text
    property color fillColor: "#1E293BCC"
    property color fillHoverColor: "#0F172ACC"
    property color fillPressedColor: AppPalette.bgDeep
    property color borderColor: AppPalette.border
    property color borderHoverColor: AppPalette.borderHover
    property real borderWidth: 1
    property bool enabled: true
    property bool rounded: true
    property real cornerRadius: rounded ? Math.min(width, height) / 2 : 10
    property int cursorShape: Qt.PointingHandCursor
    property int focusPolicy: Qt.NoFocus
    property real padding: 0
    property string toolTipText: ""
    property bool autoToolTip: true
    property real hoverWhiteness: 0.12
    readonly property bool hovered: hitArea.containsMouse
    readonly property bool pressed: hitArea.pressed
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
            return "Close"
        if (glyph === "\u2261")
            return "Menu"
        if (glyph.indexOf("\u2699") !== -1)
            return "Settings"
        if (glyph === "P")
            return "Assign popup"
        if (glyph === "2D" || glyph === "3D")
            return "Pane mode"
        if (glyph === "MOVE")
            return "Move pane"
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

    Image {
        anchors.centerIn: parent
        visible: root.hasIcon
        source: root.iconSource
        width: root.iconPixelSize
        height: root.iconPixelSize
        sourceSize.width: Math.max(1, Math.round(width * Screen.devicePixelRatio))
        sourceSize.height: Math.max(1, Math.round(height * Screen.devicePixelRatio))
        fillMode: Image.PreserveAspectFit
        cache: true
        asynchronous: false
        smooth: false
        mipmap: false
        opacity: root.enabled ? 1.0 : 0.7
    }

    Text {
        anchors.centerIn: parent
        visible: !root.hasIcon || root.showGlyphWithIcon
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
        onClicked: root.clicked()
    }

    KToolTip {
        text: root.resolvedToolTipText
        targetItem: root
        shown: root.hovered && root.enabled
    }
}
