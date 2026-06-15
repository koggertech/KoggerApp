import QtQuick 2.15
import kqml_types 1.0
import "../scene2d"

// 3D toolbar layer button (bottom track / surface / mosaic). Click opens a
// small capsule UPWARD (styled like the 2D theme switcher) with a visibility
// toggle + a gear that drills into the full settings group. For layers with a
// colormap (isobaths/mosaic) a theme swatch picker (KThemeSwitcher, opens
// RIGHT) sits at the top of the capsule. Avoids opening the big settings panel
// just to flip visibility or change theme.
Item {
    id: root

    property int buttonSize: Math.round(40 * (theme ? theme.resCoeff : 1.0))
    property string iconSource: ""
    property string toolTipText: ""
    property bool active: false        // layer visibility (bound to a store prop)
    property bool pulse: false         // processing pulse
    property bool menuOpen: false

    // Optional theme picker (isobaths / mosaic).
    property bool hasTheme: false
    property int themeCount: 0
    property int currentThemeId: 0
    property var themeNames: []
    property var themeStopsFn: null    // function(id) -> [{pos, color}]
    property real themeStripMaxWidth: 0 // cap so the right-opening strip fits

    readonly property bool menuHovered: (menuOpen && capsuleHover.hovered)
                                        || (hasTheme && themeSwitcher.menuHovered)

    signal toggleRequested()           // flip visibility
    signal settingsRequested()         // open the full settings group
    signal themePicked(int index)

    readonly property real _s: theme ? theme.resCoeff : 1.0
    readonly property int _pad: Math.round(5 * _s)
    readonly property int gap: Math.round(6 * AppPalette.scale)
    readonly property int _slots: hasTheme ? 4 : 3   // trigger + visibility + gear [+ theme]
    readonly property real _openH: _pad * 2 + buttonSize * _slots + gap * (_slots - 1)

    width: buttonSize
    height: buttonSize

    Rectangle {
        id: backing
        z: -1
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: -root._pad
        width: root.buttonSize + root._pad * 2
        height: root.menuOpen ? root._openH : 0
        radius: width / 2
        color: AppPalette.bg
        border.width: 1
        border.color: AppPalette.border
        opacity: root.menuOpen ? 1 : 0
        visible: opacity > 0.01
        clip: true

        Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
        Behavior on opacity { NumberAnimation { duration: 170; easing.type: Easing.OutCubic } }

        HoverHandler { id: capsuleHover }

        // gear (top) + visibility toggle. The theme swatch slot (just above the
        // trigger) is occupied by the root-level KThemeSwitcher (kept outside the
        // clip so its right-opening strip isn't cut).
        Column {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: root._pad
            spacing: root.gap

            KCircleIconButton {
                width: root.buttonSize
                height: root.buttonSize
                iconSource: "qrc:/icons/ui/settings.svg"
                iconTintColor: AppPalette.text
                fillColor: AppPalette.card
                fillHoverColor: AppPalette.cardHover
                borderColor: AppPalette.border
                toolTipText: qsTr("Settings")
                onClicked: {
                    root.menuOpen = false
                    root.settingsRequested()
                }
            }

            // Reserves the theme slot (between settings and visibility) — the
            // KThemeSwitcher itself sits at root over this slot (see below).
            Item {
                width: root.buttonSize
                height: root.buttonSize
                visible: root.hasTheme
            }

            KCircleIconButton {
                width: root.buttonSize
                height: root.buttonSize
                iconSource: root.active ? "qrc:/icons/ui/eye.svg" : "qrc:/icons/ui/eye-off.svg"
                iconTintColor: AppPalette.text
                fillColor:      root.active ? AppPalette.accentBgStrong : AppPalette.card
                fillHoverColor: root.active ? AppPalette.accentBorder : AppPalette.cardHover
                borderColor:    root.active ? AppPalette.accentBorder : AppPalette.border
                borderWidth:    root.active ? 2 : 1
                toolTipText: root.active ? qsTr("Hide") : qsTr("Show")
                onClicked: root.toggleRequested()
            }
        }
    }

    // Theme swatch picker — at root (NOT inside the clipped capsule) so its
    // right-opening strip is not clipped. Sits at the top slot of the pill.
    KThemeSwitcher {
        id: themeSwitcher
        visible: root.hasTheme && opacity > 0.01
        opacity: (root.hasTheme && root.menuOpen) ? 1 : 0
        z: 1
        x: 0
        // Theme slot = between visibility and settings (slot 2). Closed it sits
        // one slot lower (behind, near visibility) at opacity 0, then slides up
        // + fades in — same "emerge" feel as the clip-revealed buttons.
        y: root.menuOpen ? -(root.buttonSize + root.gap) * 2 : -(root.buttonSize + root.gap)
        buttonSize: root.buttonSize
        themeCount: root.themeCount
        currentId: root.currentThemeId
        themeNames: root.themeNames
        stopsFor: root.themeStopsFn
        maxStripWidth: root.themeStripMaxWidth
        onPicked: function(index) { root.themePicked(index) }

        Behavior on y       { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
        Behavior on opacity { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
    }

    Connections {
        target: root
        function onMenuOpenChanged() {
            if (!root.menuOpen)
                themeSwitcher.menuOpen = false
        }
    }

    KCircleIconButton {
        id: trigger
        anchors.fill: parent
        iconSource: root.iconSource
        iconTintColor: AppPalette.text
        fillHoverColor: AppPalette.cardHover
        fillColor:   root.menuOpen ? "transparent" : (root.active ? AppPalette.accentBgStrong : AppPalette.card)
        borderColor: root.active ? AppPalette.accentBorder : AppPalette.border
        borderWidth: root.menuOpen ? 0 : (root.active ? 2 : 1)
        toolTipText: root.toolTipText
        onClicked: root.menuOpen = !root.menuOpen

        SequentialAnimation {
            running: root.pulse
            loops: Animation.Infinite
            NumberAnimation { target: trigger; property: "opacity"; to: 0.2; duration: 500 }
            NumberAnimation { target: trigger; property: "opacity"; to: 1.0; duration: 500 }
        }
    }

    onPulseChanged: if (!pulse) trigger.opacity = 1.0
}
