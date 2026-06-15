import QtQuick 2.15
import QtQuick.Controls 2.15

// Unified combobox used across the app.
// - Bordered card matches KSpinBox / Connection input style.
// - Rounded popup with inset highlight.
// - Optional displayTextOverride for cases where the visible text is driven
//   by an external source (e.g. live device baudrate) instead of currentIndex.
Item {
    id: root

    property var model
    property int currentIndex: 0
    property string displayTextOverride: ""
    property int fontPixelSize: Tokens.fontMd
    property bool bold: true
    property int maxVisibleItems: 8
    property int radius: Tokens.radiusMd
    // Optional: function(index) -> [{pos, color}] colormap stops. When set, a
    // small gradient dot is drawn before each item's text (and in the field).
    property var swatchFor: null
    readonly property int swatchSize: Math.round(18 * AppPalette.scale)

    readonly property string currentText: combo.currentText
    readonly property alias hovered: combo.hovered
    readonly property alias popup: combo.popup
    readonly property alias delegateModel: combo.delegateModel

    signal activated(int index)

    // Colormap gradient circle (same look as the theme-switcher swatches).
    component ThemeDot: Canvas {
        property var stops: []
        antialiasing: true
        onStopsChanged: requestPaint()
        onWidthChanged: requestPaint()
        Component.onCompleted: requestPaint()
        onPaint: {
            var ctx = getContext("2d")
            if (!ctx) return
            ctx.reset()
            if (!stops || stops.length < 2) return
            var cx = width / 2, cy = height / 2, r = Math.min(cx, cy) - 1
            if (r <= 0) return
            var g = ctx.createLinearGradient(0, height, 0, 0)
            for (var i = 0; i < stops.length; ++i)
                g.addColorStop(stops[i].pos, stops[i].color)
            ctx.fillStyle = g
            ctx.beginPath(); ctx.arc(cx, cy, r, 0, 2 * Math.PI); ctx.fill()
            ctx.strokeStyle = AppPalette.border; ctx.lineWidth = 1
            ctx.beginPath(); ctx.arc(cx, cy, r, 0, 2 * Math.PI); ctx.stroke()
        }
    }

    implicitWidth: Math.round(120 * AppPalette.scale)
    implicitHeight: Tokens.controlHMd

    function syncFromExternalIndex() {
        if (combo.currentIndex !== root.currentIndex)
            combo.currentIndex = root.currentIndex
    }

    onCurrentIndexChanged: syncFromExternalIndex()
    onModelChanged: Qt.callLater(syncFromExternalIndex)

    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: AppPalette.bg
        border.width: 1
        border.color: combo.activeFocus
                      ? AppPalette.accentBorder
                      : (combo.hovered ? AppPalette.borderHover : AppPalette.border)

        ComboBox {
            id: combo
            anchors.fill: parent
            model: root.model
            currentIndex: root.currentIndex
            hoverEnabled: true
            focusPolicy: Qt.StrongFocus   // Tab-reachable; Space/Enter opens, arrows pick
            font.pixelSize: root.fontPixelSize

            background: Rectangle { color: "transparent"; border.width: 0 }

            contentItem: Item {
                ThemeDot {
                    id: fieldDot
                    visible: root.swatchFor !== null
                    width: visible ? root.swatchSize : 0
                    height: root.swatchSize
                    anchors.left: parent.left
                    anchors.leftMargin: Tokens.spaceSm
                    anchors.verticalCenter: parent.verticalCenter
                    stops: (root.swatchFor && combo.currentIndex >= 0) ? root.swatchFor(combo.currentIndex) : []
                }
                Text {
                    anchors.left: fieldDot.visible ? fieldDot.right : parent.left
                    anchors.leftMargin: fieldDot.visible ? Tokens.spaceXs : Tokens.spaceSm
                    anchors.right: parent.right
                    // Just enough to clear the chevron indicator at the right edge.
                    anchors.rightMargin: Math.round(12 * AppPalette.scale) + 2 * Tokens.spaceXs
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.displayTextOverride !== "" ? root.displayTextOverride : combo.displayText
                    color: AppPalette.text
                    font.pixelSize: root.fontPixelSize
                    font.bold: root.bold
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }

            indicator: Image {
                anchors.right: parent.right
                anchors.rightMargin: Tokens.spaceSm
                anchors.verticalCenter: parent.verticalCenter
                width: Math.round(12 * AppPalette.scale)
                height: Math.round(12 * AppPalette.scale)
                source: "qrc:/icons/ui/chevron-down.svg"
                fillMode: Image.PreserveAspectFit
                smooth: true
            }

            delegate: ItemDelegate {
                width: combo.popup.width - 2 * Tokens.spaceXs
                height: Tokens.controlHMd
                contentItem: Item {
                    ThemeDot {
                        id: itemDot
                        visible: root.swatchFor !== null
                        width: visible ? root.swatchSize : 0
                        height: root.swatchSize
                        anchors.left: parent.left
                        anchors.leftMargin: Tokens.spaceMd
                        anchors.verticalCenter: parent.verticalCenter
                        stops: root.swatchFor ? root.swatchFor(index) : []
                    }
                    Text {
                        anchors.left: itemDot.visible ? itemDot.right : parent.left
                        anchors.leftMargin: itemDot.visible ? Tokens.spaceSm : Tokens.spaceMd
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData
                        color: AppPalette.text
                        font.pixelSize: root.fontPixelSize
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                }
                // Inset rounded highlight so it doesn't poke into popup corners.
                background: Rectangle {
                    color: highlighted ? AppPalette.accentBg : "transparent"
                    radius: Tokens.radiusSm
                }
                highlighted: combo.highlightedIndex === index
            }

            popup: Popup {
                readonly property int itemHeight: Tokens.controlHMd

                y: combo.height + Tokens.spaceXxs
                width: combo.width
                implicitHeight: Math.min(contentItem.implicitHeight,
                                         itemHeight * root.maxVisibleItems)
                                + 2 * Tokens.spaceXs
                padding: Tokens.spaceXs

                background: Rectangle {
                    color: AppPalette.bgDeep
                    border.color: AppPalette.border
                    border.width: 1
                    radius: Tokens.radiusMd
                }
                contentItem: ListView {
                    id: comboListView
                    clip: true
                    implicitHeight: contentHeight
                    model: combo.popup.visible ? combo.delegateModel : null
                    currentIndex: combo.highlightedIndex
                    highlightMoveDuration: 0
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollIndicator.vertical: ScrollIndicator {}
                }
                onOpened: comboListView.positionViewAtIndex(combo.currentIndex, ListView.Contain)
            }

            onActivated: function(idx) {
                if (root.currentIndex !== idx)
                    root.currentIndex = idx
                root.activated(idx)
            }
        }
    }
}
