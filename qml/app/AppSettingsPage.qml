import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import kqml_types 1.0

Column {
    id: root

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: 10
    readonly property real groupWidth: Math.max(0, width)

    ConnectionsSettingsPage {
        width: root.groupWidth
        store: root.store
    }

    SettingsGroup {
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: "Workspace Layout"
        stateStore: root.store
        stateKey: "app.layoutPlacement"

        KSwitch {
            width: parent.width
            text: "Edit"
            checked: root.store.editableMode
            onToggled: {
                root.store.editableMode = checked
            }
        }

        KSwitch {
            width: parent.width
            text: "Workspace shift"
            checked: root.store.settingsPushContent
            onToggled: {
                root.store.settingsPushContent = checked
            }
        }

        KSwitch {
            width: parent.width
            text: "Global pop-up"
            checked: root.store.globalPopupEnabled
            onToggled: {
                root.store.globalPopupEnabled = checked
            }
        }

        Column {
            width: parent.width
            spacing: 8

            Text {
                text: "Sidebar position:"
                color: "#CBD5E1"
                font.pixelSize: 14
            }

            KTabBar {
                width: parent.width
                options: [
                    { label: "Left", value: "left" },
                    { label: "Right", value: "right" }
                ]
                currentValue: root.store.settingsSide
                onValueSelected: function(value) {
                    root.store.settingsSide = value
                }
            }
        }

        KButton {
            width: parent.width
            text: "Reset workspace"
            onClicked: root.store.resetWindowConfiguration()
        }

        Row {
            spacing: 10

            KButton {
                width: 36
                height: 36
                text: root.store.currentLayoutIsFavorite ? "\u2605" : "\u2606"
                checkable: true
                checked: root.store.currentLayoutIsFavorite
                fontPixelSize: 20
                onClicked: root.store.toggleCurrentLayoutFavorite()
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.store.currentLayoutIsFavorite
                      ? "Current layout is in favorites"
                      : "Add current layout to favorites"
                color: "#CBD5E1"
                font.pixelSize: 14
            }
        }

        Text {
            text: "Favorite layouts"
            color: "#E2E8F0"
            font.pixelSize: 15
            font.bold: true
        }

        Text {
            visible: root.store.favoriteLayouts.length === 0
            text: "No favorite layouts yet"
            color: "#94A3B8"
            font.pixelSize: 12
        }

        Repeater {
            model: root.store.favoriteLayouts.length

            delegate: Item {
                id: favoriteCard
                required property int index
                readonly property int favoriteIndex: index
                readonly property var favoriteEntry: (favoriteIndex >= 0 && favoriteIndex < root.store.favoriteLayouts.length)
                                                 ? root.store.favoriteLayouts[favoriteIndex]
                                                 : null
                readonly property var snapshot: favoriteEntry && favoriteEntry.layout ? favoriteEntry.layout : favoriteEntry
                readonly property var popupLinks: favoriteEntry && favoriteEntry.popupLinks ? favoriteEntry.popupLinks : []
                readonly property bool selected: root.store.favoriteLayoutIsCurrent(favoriteIndex)

                width: parent.width
                height: favoriteCardView.implicitHeight

                FavoriteLayoutCard {
                    id: favoriteCardView
                    anchors.fill: parent
                    snapshot: favoriteCard.snapshot
                    popupLinks: favoriteCard.popupLinks
                    favoriteIndex: favoriteCard.favoriteIndex
                    selected: favoriteCard.selected
                    showText: true
                    onClicked: root.store.applyFavoriteLayout(favoriteCard.favoriteIndex)
                }

                CircleIconButton {
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.topMargin: 6
                    anchors.rightMargin: 6
                    width: 24
                    height: 24
                    iconSource: ""
                    glyph: "\u00D7"
                    glyphPixelSize: 16
                    glyphColor: "#CBD5E1"
                    fillColor: "#1E293B"
                    fillHoverColor: "#172133"
                    fillPressedColor: "#0B1220"
                    borderColor: "#334155"
                    borderHoverColor: "#475569"
                    showGlyphWithIcon: true
                    toolTipText: "Remove favorite"
                    onClicked: root.store.removeFavoriteLayoutAt(favoriteIndex)
                    z: 6
                }
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: "#334155"
        }

        Text {
            text: "Layout presets"
            color: "#E2E8F0"
            font.pixelSize: 15
            font.bold: true
        }

        Repeater {
            model: [
                {
                    presetId: 1,
                    title: "Preset 1",
                    subtitle: "2 top panes, 1 bottom pane"
                },
                {
                    presetId: 2,
                    title: "Preset 2",
                    subtitle: "2 x 2 grid"
                },
                {
                    presetId: 3,
                    title: "Preset 3",
                    subtitle: "1 top pane, 2 bottom panes"
                }
            ]

            delegate: Rectangle {
                required property var modelData
                readonly property var preset: modelData
                readonly property bool hovered: cardMouse.containsMouse

                width: parent.width
                height: 88
                radius: 8
                color: hovered ? "#0F172A" : "#1E293B"
                border.width: 1
                border.color: hovered ? "#475569" : "#334155"

                Row {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 10

                    Rectangle {
                        width: 84
                        height: 64
                        radius: 6
                        color: "#0B1220"
                        border.width: 1
                        border.color: "#334155"

                        Canvas {
                            anchors.fill: parent
                            anchors.margins: 4

                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.clearRect(0, 0, width, height)

                                var gap = 4
                                var iw = width
                                var ih = height

                                function pane(x, y, w, h, mode) {
                                    var is3D = mode === "3D"
                                    var is2D = mode === "2D"
                                    ctx.fillStyle = is3D ? "#16A34A"
                                                         : is2D ? "#2563EB"
                                                                : "rgba(0, 0, 0, 0)"
                                    ctx.strokeStyle = is3D ? "#86EFAC"
                                                           : is2D ? "#93C5FD"
                                                                  : "#64748B"
                                    ctx.lineWidth = 1
                                    ctx.fillRect(x, y, w, h)
                                    ctx.strokeRect(x + 0.5, y + 0.5, w - 1, h - 1)
                                }

                                if (preset.presetId === 1) {
                                    var tH1 = ih * 0.5 - gap / 2
                                    var bH1 = ih - tH1 - gap
                                    var lW1 = iw * 0.5 - gap / 2
                                    var rW1 = iw - lW1 - gap
                                    pane(0, 0, lW1, tH1)
                                    pane(lW1 + gap, 0, rW1, tH1)
                                    pane(0, tH1 + gap, iw, bH1)
                                } else if (preset.presetId === 2) {
                                    var lW2 = iw * 0.5 - gap / 2
                                    var rW2 = iw - lW2 - gap
                                    var tH2 = ih * 0.5 - gap / 2
                                    var bH2 = ih - tH2 - gap
                                    pane(0, 0, lW2, tH2)
                                    pane(lW2 + gap, 0, rW2, tH2)
                                    pane(0, tH2 + gap, lW2, bH2)
                                    pane(lW2 + gap, tH2 + gap, rW2, bH2)
                                } else {
                                    var tH3 = ih * 0.5 - gap / 2
                                    var bH3 = ih - tH3 - gap
                                    var lW3 = iw * 0.5 - gap / 2
                                    var rW3 = iw - lW3 - gap
                                    pane(0, 0, iw, tH3)
                                    pane(0, tH3 + gap, lW3, bH3)
                                    pane(lW3 + gap, tH3 + gap, rW3, bH3)
                                }
                            }
                        }
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.max(0, parent.width - 84 - 10)
                        spacing: 4

                        Text {
                            text: preset.title
                            color: "#E2E8F0"
                            font.pixelSize: 14
                            font.bold: true
                        }

                        Text {
                            text: preset.subtitle
                            color: "#94A3B8"
                            font.pixelSize: 12
                        }
                    }
                }

                MouseArea {
                    id: cardMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.store.applyLayoutPreset(preset.presetId)
                }
            }
        }

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            text: "After applying a preset, choose 2D or 3D mode for each pane."
            color: "#94A3B8"
            font.pixelSize: 12
        }
    }

    SettingsGroup {
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: "Hotkeys Window"
        stateStore: root.store
        stateKey: "app.hotkeysWindow"

        KSwitch {
            width: parent.width
            text: "Show favorite layouts"
            checked: root.store.quickActionFavoritesEnabled
            highlighted: root.store.hotkeysRevealKey === "layouts"
            flashToken: root.store.hotkeysRevealNonce
            onToggled: {
                root.store.quickActionFavoritesEnabled = checked
                root.store.requestHotkeysReveal("layouts")
            }
        }

        KSwitch {
            width: parent.width
            text: "Show marker tool"
            checked: root.store.quickActionMarkerEnabled
            highlighted: root.store.hotkeysRevealKey === "marker"
            flashToken: root.store.hotkeysRevealNonce
            onToggled: {
                root.store.quickActionMarkerEnabled = checked
                root.store.requestHotkeysReveal("marker")
            }
        }

        KSwitch {
            width: parent.width
            text: "Show connection status"
            checked: root.store.quickActionConnectionStatusEnabled
            highlighted: root.store.hotkeysRevealKey === "connections"
            flashToken: root.store.hotkeysRevealNonce
            onToggled: {
                root.store.quickActionConnectionStatusEnabled = checked
                root.store.requestHotkeysReveal("connections")
            }
        }
    }
}
