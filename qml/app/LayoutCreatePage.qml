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

    KIsland {
        title: qsTr("Copy existing layout")

        Repeater {
            model: page.store.layouts.length

            delegate: KIslandRow {
                id: sourceRow
                required property int index

                readonly property int layoutIndex: index
                readonly property var layoutEntry: (layoutIndex >= 0 && layoutIndex < page.store.layouts.length) ? page.store.layouts[layoutIndex] : null
                readonly property bool selected: layoutIndex === page.store.activeLayoutIndex

                label: qsTr("Layout %1").arg(layoutIndex + 1)
                labelColor: selected ? "#FDE68A" : AppPalette.textStrong
                caption: selected ? qsTr("Active") : ""
                fillColor: selected ? Qt.rgba(0.98, 0.80, 0.08, AppPalette.isDark ? 0.14 : 0.20) : "transparent"
                verticalPadding: Tokens.spaceSm
                interactive: true
                onClicked: page.store.createLayoutFromLayout(sourceRow.layoutIndex)

                leading: LayoutSnapshotPreview {
                    width: Math.round(84 * AppPalette.scale)
                    height: Math.round(64 * AppPalette.scale)
                    layoutSnapshot: sourceRow.layoutEntry && sourceRow.layoutEntry.layout ? sourceRow.layoutEntry.layout : sourceRow.layoutEntry
                    popupLinks: sourceRow.layoutEntry && sourceRow.layoutEntry.popupLinks ? sourceRow.layoutEntry.popupLinks : []
                    redrawDebounceMs: 48
                }
            }
        }
    }

    KIsland {
        title: qsTr("Layout presets")

        Repeater {
            model: [
                { presetId: 4, title: qsTr("Single window"), subtitle: qsTr("One pane") },
                { presetId: 5, title: qsTr("Two windows"), subtitle: qsTr("Side by side") },
                { presetId: 1, title: qsTr("Three windows"), subtitle: qsTr("2 top panes, 1 bottom pane") },
                { presetId: 2, title: qsTr("Four windows"), subtitle: qsTr("2 × 2 grid") }
            ]

            delegate: KIslandRow {
                id: presetRow
                required property var modelData

                readonly property var preset: modelData

                label: preset.title
                caption: preset.subtitle
                verticalPadding: Tokens.spaceSm
                interactive: true
                onClicked: page.store.createLayoutFromPreset(presetRow.preset.presetId)

                leading: Rectangle {
                    width: Math.round(84 * AppPalette.scale)
                    height: Math.round(64 * AppPalette.scale)
                    radius: Tokens.radiusMd
                    color: AppPalette.bgDeep
                    border.width: 1
                    border.color: AppPalette.border

                    Item {
                        id: previewArea
                        anchors.fill: parent; anchors.margins: Tokens.spaceXs
                        readonly property real gap: 4

                        Repeater {
                            model: {
                                var iw = previewArea.width, ih = previewArea.height, g = previewArea.gap
                                if (iw <= 0 || ih <= 0) return []
                                if (presetRow.preset.presetId === 4) {
                                    return [ {x:0,y:0,w:iw,h:ih} ]
                                } else if (presetRow.preset.presetId === 5) {
                                    var lW5 = iw*.5-g/2, rW5 = iw-lW5-g
                                    return [ {x:0,y:0,w:lW5,h:ih}, {x:lW5+g,y:0,w:rW5,h:ih} ]
                                } else if (presetRow.preset.presetId === 1) {
                                    var tH = ih*.5-g/2, bH = ih-tH-g, lW = iw*.5-g/2, rW = iw-lW-g
                                    return [ {x:0,y:0,w:lW,h:tH}, {x:lW+g,y:0,w:rW,h:tH}, {x:0,y:tH+g,w:iw,h:bH} ]
                                } else if (presetRow.preset.presetId === 2) {
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
            }
        }
    }
}
