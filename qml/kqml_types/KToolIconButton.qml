import QtQuick
import QtQuick.Shapes
import kqml_types 1.0

KCircleIconButton {
    id: root

    property bool armed: false
    property string tone: "normal"
    property string badge: ""
    property color badgeColor: AppPalette.accentBgStrong
    property color badgeTextColor: AppPalette.accentText
    property real progress: -1
    property bool plusBadge: false

    readonly property bool _ok: tone === "ok"
    readonly property bool _danger: tone === "danger"
    readonly property bool _accent: armed || tone === "accent"
    readonly property real _ring: Math.max(2, Math.round(Math.min(width, height) * 0.08))
    readonly property real _ringR: (Math.min(width, height) - _ring) / 2

    implicitWidth: Tokens.controlHMd
    implicitHeight: Tokens.controlHMd
    iconTintColor: !enabled ? AppPalette.textMuted : _danger ? AppPalette.dangerText : AppPalette.text
    glyphColor: iconTintColor
    fillColor: _danger ? AppPalette.dangerBg
             : _ok ? AppPalette.linkOkBg
             : _accent ? AppPalette.accentBgStrong
             : AppPalette.card
    fillHoverColor: _danger ? AppPalette.dangerHover
                  : _ok ? Qt.lighter(AppPalette.linkOkBg, 1.25)
                  : _accent ? AppPalette.accentBorder
                  : AppPalette.cardHover
    fillPressedColor: _danger ? Qt.darker(AppPalette.dangerBg, 1.15)
                    : _ok ? Qt.darker(AppPalette.linkOkBg, 1.15)
                    : AppPalette.bgHover
    borderColor: _danger ? AppPalette.dangerBorder
               : _ok ? AppPalette.linkOkBorder
               : _accent ? AppPalette.accentBorder
               : AppPalette.border
    borderHoverColor: _danger ? AppPalette.dangerBorder
                    : _ok ? AppPalette.linkOkBorder
                    : _accent ? AppPalette.accentBorder
                    : AppPalette.borderHover
    borderWidth: 1

    Shape {
        anchors.fill: parent
        visible: root.progress >= 0
        scale: root.backgroundScale
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeColor: AppPalette.border
            strokeWidth: root._ring
            fillColor: "transparent"
            PathAngleArc {
                centerX: root.width / 2; centerY: root.height / 2
                radiusX: root._ringR; radiusY: root._ringR
                startAngle: 0; sweepAngle: 360
            }
        }
        ShapePath {
            strokeColor: AppPalette.accent
            strokeWidth: root._ring
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            PathAngleArc {
                centerX: root.width / 2; centerY: root.height / 2
                radiusX: root._ringR; radiusY: root._ringR
                startAngle: -90
                sweepAngle: 360 * Math.max(0, Math.min(1, root.progress))
            }
        }
    }

    HoverHandler {
        id: disabledHover
        enabled: !root.enabled
    }

    KToolTip {
        text: root.resolvedToolTipText
        targetItem: root
        shown: !root.enabled && disabledHover.hovered && !root.toolTipSuppressed
    }

    KCloseBadge {
        visible: root.plusBadge
        glyphRotation: 0
        fillColor: root.enabled ? AppPalette.accentBgStrong : AppPalette.textMuted
        crossColor: AppPalette.accentText
        ringColor: AppPalette.bg
        anchors.rightMargin: -Math.round(width * 0.2)
        anchors.bottomMargin: -Math.round(width * 0.2)
    }

    Rectangle {
        visible: root.badge.length > 0
        readonly property int _h: Math.max(Math.round(13 * AppPalette.scale), Math.round(root.height * 0.42))
        height: _h
        width: Math.max(_h, badgeLabel.implicitWidth + Math.round(_h * 0.5))
        radius: _h / 2
        x: root.width - width + Math.round(_h * 0.35)
        y: -Math.round(_h * 0.3)
        color: root.badgeColor
        border.width: 1
        border.color: AppPalette.bg
        Text {
            id: badgeLabel
            anchors.centerIn: parent
            text: root.badge
            color: root.badgeTextColor
            font.pixelSize: Math.max(Tokens.fontXxs - 1, Math.round(parent.height * 0.68))
            font.bold: true
        }
    }
}
