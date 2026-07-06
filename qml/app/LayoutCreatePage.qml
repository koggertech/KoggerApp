import QtQuick 2.15
import kqml_types 1.0

Column {
    id: page

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

    Text {
        width: parent.width; wrapMode: Text.WordWrap
        text: qsTr("Create a new layout by copying an existing one (the active one is marked) or from a preset, then arrange it in edit mode.")
        color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm
    }

    Text { text: qsTr("Copy existing layout"); color: AppPalette.text; font.pixelSize: Tokens.fontLg; font.bold: true }

    Repeater {
        model: page.store.layouts.length
        delegate: Item {
            id: srcCard
            required property int index
            readonly property int layoutIndex: index
            readonly property var layoutEntry: (layoutIndex >= 0 && layoutIndex < page.store.layouts.length) ? page.store.layouts[layoutIndex] : null
            readonly property var snapshot: layoutEntry && layoutEntry.layout ? layoutEntry.layout : layoutEntry
            readonly property var popupLinks: layoutEntry && layoutEntry.popupLinks ? layoutEntry.popupLinks : []
            readonly property bool selected: layoutIndex === page.store.activeLayoutIndex
            width: parent.width; height: srcCardView.implicitHeight

            FavoriteLayoutCard {
                id: srcCardView
                anchors.fill: parent
                snapshot: srcCard.snapshot; popupLinks: srcCard.popupLinks
                favoriteIndex: srcCard.layoutIndex; selected: srcCard.selected; showText: true
                onClicked: page.store.createLayoutFromLayout(srcCard.layoutIndex)
            }
        }
    }

    Text { text: qsTr("Layout presets"); color: AppPalette.text; font.pixelSize: Tokens.fontLg; font.bold: true }

    Repeater {
        model: [
            { presetId: 4, title: qsTr("Single window"), subtitle: qsTr("One pane") },
            { presetId: 5, title: qsTr("Two windows"), subtitle: qsTr("Side by side") },
            { presetId: 1, title: qsTr("Three windows"), subtitle: qsTr("2 top panes, 1 bottom pane") },
            { presetId: 2, title: qsTr("Four windows"), subtitle: qsTr("2 × 2 grid") }
        ]
        delegate: Rectangle {
            id: presetCard
            required property var modelData
            readonly property var preset: modelData
            readonly property bool hovered: cardMouse.containsMouse
            width: parent.width; height: Math.round(88 * AppPalette.scale); radius: Tokens.radiusLg
            color: hovered ? AppPalette.cardHover : AppPalette.card; border.width: Tokens.cardBorderWidth
            border.color: hovered ? AppPalette.borderHover : AppPalette.border

            activeFocusOnTab: true
            Keys.onReturnPressed: page.store.createLayoutFromPreset(preset.presetId)
            Keys.onEnterPressed:  page.store.createLayoutFromPreset(preset.presetId)
            Keys.onSpacePressed:  page.store.createLayoutFromPreset(preset.presetId)

            Row {
                anchors.fill: parent; anchors.margins: Tokens.spaceMd; spacing: Tokens.spaceLg
                Rectangle {
                    width: Math.round(84 * AppPalette.scale); height: Math.round(64 * AppPalette.scale); radius: Tokens.radiusMd; color: AppPalette.bgDeep
                    border.width: 1; border.color: AppPalette.border
                    Item {
                        id: previewArea
                        anchors.fill: parent; anchors.margins: Tokens.spaceXs
                        readonly property real gap: 4
                        Repeater {
                            model: {
                                var iw = previewArea.width, ih = previewArea.height, g = previewArea.gap
                                if (iw <= 0 || ih <= 0) return []
                                if (preset.presetId === 4) {
                                    return [ {x:0,y:0,w:iw,h:ih} ]
                                } else if (preset.presetId === 5) {
                                    var lW5 = iw*.5-g/2, rW5 = iw-lW5-g
                                    return [ {x:0,y:0,w:lW5,h:ih}, {x:lW5+g,y:0,w:rW5,h:ih} ]
                                } else if (preset.presetId === 1) {
                                    var tH = ih*.5-g/2, bH = ih-tH-g, lW = iw*.5-g/2, rW = iw-lW-g
                                    return [ {x:0,y:0,w:lW,h:tH}, {x:lW+g,y:0,w:rW,h:tH}, {x:0,y:tH+g,w:iw,h:bH} ]
                                } else if (preset.presetId === 2) {
                                    var lW2 = iw*.5-g/2, rW2 = iw-lW2-g, tH2 = ih*.5-g/2, bH2 = ih-tH2-g
                                    return [ {x:0,y:0,w:lW2,h:tH2}, {x:lW2+g,y:0,w:rW2,h:tH2}, {x:0,y:tH2+g,w:lW2,h:bH2}, {x:lW2+g,y:tH2+g,w:rW2,h:bH2} ]
                                }
                                return []
                            }
                            delegate: Rectangle {
                                required property var modelData
                                x: modelData.x; y: modelData.y
                                width: modelData.w; height: modelData.h
                                color: "transparent"
                                border.width: 1; border.color: "#64748B"
                            }
                        }
                    }
                }
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.max(0, parent.width - Math.round(84 * AppPalette.scale) - Tokens.spaceLg); spacing: Tokens.spaceXs
                    Text { text: preset.title; color: AppPalette.text; font.pixelSize: Tokens.fontBase; font.bold: true }
                    Text { text: preset.subtitle; color: AppPalette.textMuted; font.pixelSize: Tokens.fontSm }
                }
            }
            KFocusRing { id: focusRing }

            MouseArea { id: cardMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onPressed: focusRing.suppress(); onClicked: { presetCard.forceActiveFocus(); page.store.createLayoutFromPreset(preset.presetId) } }
        }
    }
}
