import QtQuick 2.15

// Reusable keyboard focus ring for Item-based controls. Encapsulates both the
// ring visual and the "show only on keyboard focus" suppression (mouse click
// must NOT draw a ring). Replaces the per-control `_ringSuppressed` boilerplate.
//
//   Rectangle { id: ctl
//       activeFocusOnTab: true
//       Keys.onReturnPressed: doThing()
//       KFocusRing { id: focusRing }            // anchors to + tracks parent
//       MouseArea {
//           onPressed: focusRing.suppress()     // focus via mouse → no ring
//           onClicked: { ctl.forceActiveFocus(); doThing() }
//       }
//   }
//
// • `target`    — Item the ring wraps (size + radius). Default: parent.
// • `focusItem` — Item whose activeFocus drives visibility. Default: target.
//                 Set explicitly when the ring wraps a child but a different
//                 Item owns the focus (e.g. KCircleIconButton → backgroundRect).
// • `inset`     — outset in px (scaled); ring sits `inset` OUTSIDE target.
//
// QtQuick.Controls (KButton/KSwitch…) keep their own `visualFocus` rings — Qt
// distinguishes mouse vs keyboard focus there for free, so they need no suppression.
Rectangle {
    id: ring

    property Item target: parent
    property Item focusItem: target
    property real inset: 0

    // Set on mouse press so click-focus shows no ring; auto-reset on focus loss.
    property bool _suppressed: false
    function suppress() { ring._suppressed = true }

    Connections {
        target: ring.focusItem
        function onActiveFocusChanged() {
            if (ring.focusItem && !ring.focusItem.activeFocus)
                ring._suppressed = false
        }
    }

    readonly property real _insetPx: inset !== 0 ? Math.round(inset * AppPalette.scale) : 0

    anchors.fill: target
    anchors.margins: -_insetPx
    // Outset by `inset` → bump radius to match so a pill stays a pill.
    radius: ((target && target.radius !== undefined) ? target.radius : 0) + _insetPx
    color: "transparent"
    border.width: 2
    border.color: AppPalette.accentBorder
    visible: focusItem ? (focusItem.activeFocus && !ring._suppressed) : false
    z: 5
}
