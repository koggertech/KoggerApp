import QtQuick 2.15
import kqml_types 1.0

Column {
    id: page

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceMd

    Component.onDestruction: if (store) store.quickActionDraggingKey = ""

    readonly property int rowH: Math.round(38 * AppPalette.scale)   // == KSwitch.rowHeight: aligns burger/switch/highlight
    readonly property int handleW: Math.round(28 * AppPalette.scale)

    function _label(key) {
        return key === "connections" ? qsTr("Connected devices")
             : key === "logging"     ? qsTr("Logging")
             : key === "favorites"   ? qsTr("Favorite layouts")
             : key === "bottomTrack" ? qsTr("Bottom track editing")
             : key === "extraInfo"   ? qsTr("Extra info")
             : key === "autopilot"   ? qsTr("Autopilot")
             : key === "profiles"    ? qsTr("Profiles")
             : key === "secondWindow" ? qsTr("Second window")
             : key
    }
    function _checked(key) {
        if (!store) return false
        return key === "connections" ? store.quickActionConnectionStatusEnabled
             : key === "logging"     ? store.quickActionLoggingEnabled
             : key === "favorites"   ? store.quickActionFavoritesEnabled
             : key === "bottomTrack" ? store.quickActionBottomTrackEnabled
             : key === "extraInfo"   ? store.quickActionExtraInfoEnabled
             : key === "autopilot"   ? store.quickActionAutopilotEnabled
             : key === "profiles"    ? store.quickActionProfilesEnabled
             : key === "secondWindow" ? store.quickActionSecondWindowEnabled
             : false
    }
    function _toggle(key, v) {
        if (!store) return
        if (key === "connections") {
            store.quickActionConnectionStatusEnabled = v
            store.requestHotkeysReveal("connections")
        } else if (key === "logging") {
            store.quickActionLoggingEnabled = v
            store.requestHotkeysReveal("logging")
        } else if (key === "favorites") {
            store.quickActionFavoritesEnabled = v
            if (store.favoriteLayouts && store.favoriteLayouts.length > 0)
                store.requestHotkeysReveal("layouts")
        } else if (key === "bottomTrack") {
            store.quickActionBottomTrackEnabled = v
            store.requestHotkeysReveal("bottomTrack")
        } else if (key === "extraInfo") {
            store.quickActionExtraInfoEnabled = v
            store.requestHotkeysReveal("extraInfo")
        } else if (key === "autopilot") {
            store.quickActionAutopilotEnabled = v
            store.requestHotkeysReveal("autopilot")
        } else if (key === "profiles") {
            store.quickActionProfilesEnabled = v
            store.requestHotkeysReveal("profiles")
        } else if (key === "secondWindow") {
            store.quickActionSecondWindowEnabled = v
            store.requestHotkeysReveal("secondWindow")
        }
    }

    Text {
        width: parent.width
        text: qsTr("Toggle which items appear and drag to reorder them.")
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontSm
        wrapMode: Text.WordWrap
        bottomPadding: Tokens.spaceXs
    }

    // Dragged row reparents into dragLayer (no positioner) to float above the list.
    Item {
        id: dragLayer
        readonly property int rowCount: page.store ? page.store.quickActionOrderModel.count : 0
        width: parent.width
        height: rowCount * page.rowH + Math.max(0, rowCount - 1) * list.spacing

        ListView {
            id: list
            anchors.fill: parent
            interactive: false
            spacing: Tokens.spaceMd
            model: page.store ? page.store.quickActionOrderModel : 0
            cacheBuffer: 10000

            moveDisplaced: Transition { NumberAnimation { properties: "y"; duration: 160; easing.type: Easing.OutCubic } }

            delegate: DropArea {
                id: dropArea
                required property string key
                required property int index
                width: list.width
                height: page.rowH
                property int visualIndex: index

                onEntered: function(drag) {
                    var from = drag.source.visualIndex
                    if (from !== dropArea.visualIndex && page.store)
                        page.store.moveQuickAction(from, dropArea.visualIndex)
                }

                Rectangle {
                    id: rowContent
                    width: dropArea.width
                    height: page.rowH
                    radius: Tokens.radiusLg
                    color: dragArea.drag.active ? AppPalette.bgHover : "transparent"
                    border.width: dragArea.drag.active ? 1 : 0
                    border.color: AppPalette.border
                    property int visualIndex: dropArea.visualIndex

                    readonly property bool dragActive: dragArea.drag.active
                    onDragActiveChanged: if (page.store) page.store.quickActionDraggingKey = dragActive ? key : ""

                    Drag.active: dragArea.drag.active
                    Drag.source: rowContent
                    Drag.hotSpot.x: width / 2
                    Drag.hotSpot.y: height / 2

                    states: State {
                        when: dragArea.drag.active
                        ParentChange { target: rowContent; parent: dragLayer }
                    }

                    Row {
                        anchors.fill: parent
                        spacing: Tokens.spaceSm

                        Item {
                            width: page.handleW
                            height: parent.height
                            Column {
                                anchors.centerIn: parent
                                spacing: Math.round(3 * AppPalette.scale)
                                Repeater {
                                    model: 3
                                    Rectangle {
                                        width: Math.round(14 * AppPalette.scale)
                                        height: Math.max(2, Math.round(2 * AppPalette.scale))
                                        radius: height / 2
                                        color: dragArea.drag.active ? AppPalette.accentBar : AppPalette.textMuted
                                        Behavior on color { ColorAnimation { duration: 110; easing.type: Easing.OutCubic } }
                                    }
                                }
                            }
                            MouseArea {
                                id: dragArea
                                anchors.fill: parent
                                cursorShape: Qt.SizeVerCursor
                                drag.target: rowContent
                                drag.axis: Drag.YAxis
                                onReleased: if (page.store) page.store.persistQuickActionOrder()
                            }
                        }

                        KSwitch {
                            width: parent.width - page.handleW - parent.spacing
                            text: page._label(key)
                            checked: page._checked(key)
                            onToggled: page._toggle(key, checked)
                        }
                    }
                }
            }
        }
    }
}
