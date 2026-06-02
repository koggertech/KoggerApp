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

    readonly property string currentText: combo.currentText
    readonly property alias hovered: combo.hovered
    readonly property alias popup: combo.popup
    readonly property alias delegateModel: combo.delegateModel

    signal activated(int index)

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
            font.pixelSize: root.fontPixelSize

            background: Rectangle { color: "transparent"; border.width: 0 }

            contentItem: Text {
                leftPadding: Tokens.spaceSm
                // Just enough to clear the chevron indicator at the right edge.
                rightPadding: Math.round(12 * AppPalette.scale) + 2 * Tokens.spaceXs
                text: root.displayTextOverride !== "" ? root.displayTextOverride : combo.displayText
                color: AppPalette.text
                font.pixelSize: root.fontPixelSize
                font.bold: root.bold
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
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
                contentItem: Text {
                    text: modelData
                    color: AppPalette.text
                    font.pixelSize: root.fontPixelSize
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: Tokens.spaceMd
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
