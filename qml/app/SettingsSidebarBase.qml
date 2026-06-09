import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import kqml_types 1.0

Item {
    id: panelRoot
    z: 400

    property bool open: false
    readonly property bool pointerInside: panelHoverGuard.hovered
    property bool dimEnabled: true
    property string title: qsTr("Settings")
    property string side: "left"
    property string gearMode: "app"
    property color headerColor: AppPalette.headerBg
    property color panelColor: AppPalette.bg
    property bool panelShadowEnabled: true
    property real panelShadowOpacity: 0.72
    property int panelShadowSize: 30
    property real panelSizePx: 300
    property int scrollBarReservePx: Math.round(20 * AppPalette.scale)
    readonly property int _scrollThumbW: Math.round(12 * AppPalette.scale)
    readonly property string resolvedSide: side === "right" ? "right" : "left"
    property real progress: open ? 1.0 : 0.0
    readonly property real panelWidth: panelSizePx

    signal closeRequested()

    property bool showBack: false
    signal backRequested()

    property Component subPage: null
    property bool subPageOpen: false

    default property alias contentData: contentColumn.data

    property var store: null

    function _toggleAllGroups() {
        if (subPageOpen)
            return
        if (store && typeof store.toggleAllSettingsGroups === "function")
            store.toggleAllSettingsGroups()
        scrollToTopTimer.restart()
    }

    Timer {
        id: scrollToTopTimer
        interval: 80
        repeat: false
        onTriggered: {
            if (contentFlick.contentHeight > contentFlick.height)
                scrollToTopAnim.restart()
            else
                contentFlick.contentY = 0
        }
    }

    NumberAnimation {
        id: scrollToTopAnim
        target: contentFlick
        property: "contentY"
        to: 0
        duration: 220
        easing.type: Easing.OutCubic
    }

    visible: progress > 0.01

    Behavior on progress {
        NumberAnimation {
            duration: AppPalette.sidebarAnimMs
            easing.type: Easing.OutCubic
        }
    }

    Rectangle {
        id: shade

        anchors.fill: parent
        color: "#02061788"
        opacity: panelRoot.dimEnabled ? 0.55 * panelRoot.progress : 0.0
        z: 0
    }

    Rectangle {
        id: panel

        x: panelRoot.resolvedSide === "left"
           ? -panelRoot.panelWidth + panelRoot.panelWidth * panelRoot.progress
           : panelRoot.width - panelRoot.panelWidth * panelRoot.progress
        y: 0
        width: panelRoot.panelWidth
        height: panelRoot.height
        color: panelRoot.panelColor
        border.width: 1
        border.color: AppPalette.border
        z: 2

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            onPressed: function(mouse) { mouse.accepted = true }
            onClicked: {}
            onWheel: function(wheel) { wheel.accepted = true }
        }

        HoverHandler {
            id: panelHoverGuard
            enabled: panelRoot.progress > 0.01
        }

        Rectangle {
            id: topSection

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            color: panelRoot.headerColor
            height: topSectionContent.y + topSectionContent.implicitHeight + Tokens.spaceXl
            z: 1

            Column {
                id: topSectionContent

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.leftMargin: Tokens.spaceXl
                anchors.rightMargin: Tokens.spaceXl
                anchors.topMargin: Tokens.spaceXl
                spacing: Tokens.spaceLg

                RowLayout {
                    width: parent.width
                    height: Tokens.controlHXl - Tokens.spaceXs
                    spacing: Tokens.spaceMd

                    KButton {
                        id: backButton
                        visible: panelRoot.showBack
                        text: ""
                        Layout.preferredWidth:  visible ? Tokens.controlHLg : 0
                        Layout.maximumWidth:    visible ? Tokens.controlHLg : 0
                        Layout.preferredHeight: Tokens.controlHLg
                        Layout.minimumHeight:   Tokens.controlHLg
                        Layout.maximumHeight:   Tokens.controlHLg
                        Layout.alignment: Qt.AlignVCenter
                        onClicked: panelRoot.backRequested()

                        DisclosureIndicator {
                            anchors.centerIn: parent
                            width: Math.round(12 * AppPalette.scale)
                            height: width
                            rotation: 180
                            expanded: false
                            indicatorColor: backButton.hovered ? Qt.lighter(AppPalette.text, 1.08)
                                                               : AppPalette.text
                        }
                    }

                    Text {
                        id: titleText
                        text: panelRoot.title
                        color: AppPalette.text
                        font.pixelSize: Tokens.fontXl
                        font.bold: true
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        elide: Text.ElideRight

                        // Double-tap toggles all SettingsGroup descendants.
                        KTapArea {
                            onDoubleTapped: panelRoot._toggleAllGroups()
                        }
                    }

                    KButton {
                        text: "\u2715"
                        width: Tokens.controlHLg
                        height: Tokens.controlHLg
                        fontPixelSize: Tokens.fontXxl
                        Layout.preferredWidth: width
                        Layout.preferredHeight: height
                        Layout.minimumWidth: width
                        Layout.minimumHeight: height
                        Layout.maximumWidth: width
                        Layout.maximumHeight: height
                        Layout.alignment: Qt.AlignVCenter
                        onClicked: panelRoot.closeRequested()
                    }
                }
            }
        }

        Flickable {
            id: contentFlick

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: topSection.bottom
            anchors.bottom: parent.bottom
            anchors.leftMargin: Tokens.spaceXl
            // No right margin: the scrollbar lives in scrollBarReservePx on
            // the right side of the Flickable — pushing the Flickable further
            // left makes the scrollbar look off-centre (large gap to panel
            // edge, small gap to content). Content right padding is provided
            // entirely by the reserve column.
            anchors.rightMargin: 0
            anchors.bottomMargin: Tokens.spaceXl
            anchors.topMargin: Tokens.spaceLg

            clip: true
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick
            contentWidth: Math.max(0, width - panelRoot.scrollBarReservePx)
            contentHeight: Math.max(height, contentColumn.implicitHeight)
            interactive: contentHeight > height + 1
            z: 2

            // Attached ScrollBar.vertical: positioning intentionally NOT
            // delegated to Qt's internal layout — Qt fixes x at flickable
            // edge regardless of anchors.rightMargin, and doesn't reflow on
            // AppPalette.scale changes. We attach for behaviour, but a
            // standalone styled ScrollBar below paints/handles input.
            ScrollBar.vertical: ScrollBar {
                id: vScrollAttached
                policy: ScrollBar.AlwaysOff   // hide Qt's default thumb
            }

            Column {
                id: contentColumn

                width: Math.max(0, contentFlick.width - panelRoot.scrollBarReservePx)
                spacing: Tokens.spaceLg
            }
        }

        // ── Custom standalone vertical scrollbar ──────────────────────────
        // Reasons to avoid the attached Flickable.ScrollBar.vertical:
        //  • Qt fixes x at flickable.right and ignores anchors.rightMargin
        //    overrides — can't centre the thumb in the reserve column.
        //  • Doesn't reflow on AppPalette.scale changes (cached layout).
        // This standalone ScrollBar binds bidirectionally to contentFlick
        // via size/position and writes contentY when dragged. Full control
        // over geometry, width and styling.
        ScrollBar {
            id: vScroll
            orientation: Qt.Vertical
            policy: contentFlick.contentHeight > contentFlick.height + 1
                    ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff

            parent: panel
            anchors.top: contentFlick.top
            anchors.bottom: contentFlick.bottom
            anchors.right: contentFlick.right
            anchors.rightMargin: Math.round((panelRoot.scrollBarReservePx - panelRoot._scrollThumbW) / 2)
            width: panelRoot._scrollThumbW
            implicitWidth: panelRoot._scrollThumbW
            z: 4

            size: contentFlick.contentHeight > 0
                  ? Math.min(1, contentFlick.height / contentFlick.contentHeight) : 0
            position: contentFlick.contentHeight > 0
                      ? contentFlick.contentY / contentFlick.contentHeight : 0
            stepSize: 0.04
            active: contentFlick.movingVertically || pressed || hovered

            onPositionChanged: {
                if (vScroll.pressed) {
                    contentFlick.contentY = vScroll.position * contentFlick.contentHeight
                }
            }

            contentItem: Rectangle {
                implicitWidth: vScroll.width
                radius: width / 2
                color: vScroll.pressed
                       ? AppPalette.text
                       : (vScroll.hovered ? AppPalette.textSecond : AppPalette.textMuted)
                opacity: vScroll.pressed ? 0.85 : (vScroll.hovered ? 0.65 : 0.45)
                Behavior on color   { ColorAnimation { duration: 120 } }
                Behavior on opacity { NumberAnimation { duration: 120 } }
            }
            background: Item {}
        }

        // ── Scroll-edge fade overlays ─────────────────────────────────────
        // Cheap: GPU-rendered Rectangle gradients. They sit on top of the
        // Flickable (z: 3) and fade content into panelColor where it slides
        // out of view. Hidden when scrolled to the edge (atYBeginning/End).
        readonly property color _fadeStart: panelRoot.panelColor
        readonly property color _fadeEnd: Qt.rgba(panelRoot.panelColor.r,
                                                  panelRoot.panelColor.g,
                                                  panelRoot.panelColor.b, 0)
        readonly property int _fadeHeight: Math.round(24 * AppPalette.scale)

        Rectangle {
            id: topFade
            x: contentFlick.x
            y: contentFlick.y
            width: contentFlick.width - panelRoot.scrollBarReservePx
            height: panel._fadeHeight
            z: 3
            opacity: contentFlick.atYBeginning ? 0.0 : 1.0
            visible: opacity > 0.01
            Behavior on opacity { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }

            gradient: Gradient {
                GradientStop { position: 0.0; color: panel._fadeStart }
                GradientStop { position: 1.0; color: panel._fadeEnd }
            }
        }

        Rectangle {
            id: bottomFade
            x: contentFlick.x
            y: contentFlick.y + contentFlick.height - height
            width: contentFlick.width - panelRoot.scrollBarReservePx
            height: panel._fadeHeight
            z: 3
            opacity: contentFlick.atYEnd ? 0.0 : 1.0
            visible: opacity > 0.01
            Behavior on opacity { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }

            gradient: Gradient {
                GradientStop { position: 0.0; color: panel._fadeEnd }
                GradientStop { position: 1.0; color: panel._fadeStart }
            }
        }

        Item {
            id: subPageHost
            anchors.top: topSection.bottom
            anchors.bottom: parent.bottom
            width: panel.width
            z: 5                                   // above content (2), fades (3), scrollbar (4)
            clip: true
            x: panelRoot.subPageOpen ? 0 : -width
            visible: panelRoot.subPage !== null && (panelRoot.subPageOpen || x > -width + 0.5)

            Behavior on x {
                NumberAnimation { duration: Anim.subpageMs; easing.type: Anim.subpageEasing }
            }

            Rectangle {
                anchors.fill: parent
                color: panelRoot.panelColor
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.AllButtons
                    onPressed: function(mouse) { mouse.accepted = true }
                    onWheel: function(wheel) { wheel.accepted = true }
                }
            }

            Flickable {
                id: subFlick
                anchors.fill: parent
                anchors.leftMargin: Tokens.spaceXl
                anchors.rightMargin: 0
                anchors.topMargin: Tokens.spaceLg
                anchors.bottomMargin: Tokens.spaceXl
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.VerticalFlick
                contentWidth: Math.max(0, width - panelRoot.scrollBarReservePx)
                contentHeight: Math.max(height, subLoader.implicitHeight)
                interactive: contentHeight > height + 1
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOff }

                Loader {
                    id: subLoader
                    width: subFlick.contentWidth
                    active: panelRoot.subPage !== null
                    sourceComponent: panelRoot.subPage
                }
            }

            ScrollBar {
                id: subVScroll
                orientation: Qt.Vertical
                policy: subFlick.contentHeight > subFlick.height + 1
                        ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff

                parent: subPageHost
                anchors.top: subFlick.top
                anchors.bottom: subFlick.bottom
                anchors.right: subFlick.right
                anchors.rightMargin: Math.round((panelRoot.scrollBarReservePx - panelRoot._scrollThumbW) / 2)
                width: panelRoot._scrollThumbW
                implicitWidth: panelRoot._scrollThumbW
                z: 4

                size: subFlick.contentHeight > 0
                      ? Math.min(1, subFlick.height / subFlick.contentHeight) : 0
                position: subFlick.contentHeight > 0
                          ? subFlick.contentY / subFlick.contentHeight : 0
                stepSize: 0.04
                active: subFlick.movingVertically || pressed || hovered

                onPositionChanged: {
                    if (subVScroll.pressed)
                        subFlick.contentY = subVScroll.position * subFlick.contentHeight
                }

                contentItem: Rectangle {
                    implicitWidth: subVScroll.width
                    radius: width / 2
                    color: subVScroll.pressed
                           ? AppPalette.text
                           : (subVScroll.hovered ? AppPalette.textSecond : AppPalette.textMuted)
                    opacity: subVScroll.pressed ? 0.85 : (subVScroll.hovered ? 0.65 : 0.45)
                    Behavior on color   { ColorAnimation { duration: 120 } }
                    Behavior on opacity { NumberAnimation { duration: 120 } }
                }
                background: Item {}
            }
        }
    }

    Rectangle {
        id: panelShadow

        y: panel.y
        width: panelRoot.panelShadowSize
        height: panel.height
        x: panelRoot.resolvedSide === "left" ? panel.x + panel.width : panel.x - width
        z: 1
        visible: panelRoot.panelShadowEnabled && panelRoot.progress > 0.01
        opacity: panelRoot.panelShadowOpacity * panelRoot.progress

        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop {
                position: 0.0
                color: panelRoot.resolvedSide === "left" ? AppPalette.shadow0 : "#00000000"
            }
            GradientStop {
                position: 0.45
                color: AppPalette.shadowMid
            }
            GradientStop {
                position: 1.0
                color: panelRoot.resolvedSide === "left" ? "#00000000" : AppPalette.shadow0
            }
        }
    }
}
