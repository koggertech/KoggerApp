import QtQuick 2.15
import Qt5Compat.GraphicalEffects
import kqml_types 1.0

Column {
    id: page

    required property var store

    readonly property var buttons: [
        { key: "arm",           text: "ARM", label: qsTr("Arm / disarm") },
        { key: "mode.0",        icon: "qrc:/icons/ui/mode-manual.svg",          label: "Manual" },
        { key: "mode.1",        icon: "qrc:/icons/ui/mode-course-hold.svg",     label: "Acro" },
        { key: "mode.4",        icon: "qrc:/icons/ui/mode-hold.svg",            label: "Hold" },
        { key: "mode.5",        icon: "qrc:/icons/ui/mode-loiter.svg",          label: "Loiter" },
        { key: "mode.10",       icon: "qrc:/icons/ui/mode-auto.svg",            label: "Auto" },
        { key: "mode.11",       icon: "qrc:/icons/ui/mode-rtl.svg",             label: "RTL" },
        { key: "goTo",          icon: "qrc:/icons/ui/location_pin.svg",         label: qsTr("Go here") },
        { key: "params",        icon: "qrc:/icons/ui/adjustments_horizontal.svg", label: qsTr("Autopilot parameters") },
        { key: "missionEditor", icon: "qrc:/icons/ui/map_route.svg",            label: qsTr("Mission editor") },
        { key: "close",         icon: "qrc:/icons/ui/x.svg",                    label: qsTr("Close") }
    ]
    readonly property var panels: [
        { key: "panel.boat",     icon: "qrc:/icons/ui/speedboat.svg",         label: qsTr("Boat") },
        { key: "panel.mission",  icon: "qrc:/icons/ui/route.svg",             label: qsTr("Mission") },
        { key: "panel.settings", icon: "qrc:/icons/ui/settings.svg",          label: qsTr("Settings") },
        { key: "panel.linkLoss", icon: "qrc:/icons/ui/autopilot-link-off.svg", label: qsTr("On link loss") },
        { key: "panel.messages", icon: "qrc:/icons/ui/list-details.svg",      label: qsTr("Messages") }
    ]

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

    component ToggleRow: KIslandRow {
        id: toggleRow
        required property var modelData
        label: modelData.label
        interactive: true
        onClicked: toggleSwitch.click()

        leading: ButtonPreview {
            icon: toggleRow.modelData.icon || ""
            text: toggleRow.modelData.text || ""
        }

        KSwitch {
            id: toggleSwitch
            flat: true
            checked: page.store ? page.store.autopilotCardButtonShown(toggleRow.modelData.key) : true
            onToggled: {
                if (page.store)
                    page.store.setAutopilotCardButtonShown(toggleRow.modelData.key, checked)
                checked = Qt.binding(function() {
                    return page.store ? page.store.autopilotCardButtonShown(toggleRow.modelData.key) : true
                })
            }
        }
    }

    component ButtonPreview: Rectangle {
        id: preview
        property url icon: ""
        property string text: ""
        width: Math.round(28 * AppPalette.scale)
        height: width
        radius: width / 2
        color: AppPalette.card
        border.color: AppPalette.border
        border.width: Tokens.cardBorderWidth

        Image {
            id: previewImage
            anchors.centerIn: parent
            width: Math.round(parent.width * 0.56)
            height: width
            source: preview.icon
            sourceSize.width: Math.max(1, Math.round(width * Screen.devicePixelRatio))
            sourceSize.height: Math.max(1, Math.round(height * Screen.devicePixelRatio))
            fillMode: Image.PreserveAspectFit
            smooth: true
            visible: false
        }
        ColorOverlay {
            anchors.fill: previewImage
            source: previewImage
            color: AppPalette.text
            visible: preview.text.length === 0
        }
        Text {
            anchors.centerIn: parent
            visible: preview.text.length > 0
            text: preview.text
            color: AppPalette.textStrong
            font.pixelSize: Math.round(9 * AppPalette.scale)
            font.bold: true
        }
    }

    KIsland {
        KIslandRow {
            label: qsTr("Tutorial")
            caption: qsTr("A short tour of the autopilot card")
            chevron: true
            interactive: true
            onClicked: if (typeof tutorial !== "undefined" && tutorial) tutorial.start("autopilot")
        }
    }

    Repeater {
        model: [
            { title: qsTr("Card buttons:"), items: page.buttons },
            { title: qsTr("Panels of the expanded card:"), items: page.panels }
        ]

        delegate: Column {
            id: toggleSection
            required property var modelData
            width: page.width
            spacing: Tokens.spaceMd

            Text {
                width: parent.width
                text: toggleSection.modelData.title
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontBase
                wrapMode: Text.WordWrap
            }

            KIsland {
                Repeater {
                    model: toggleSection.modelData.items
                    delegate: ToggleRow {}
                }
            }
        }
    }

    Column {
        width: parent.width
        spacing: Tokens.spaceMd

        Text {
            text: qsTr("Card size:")
            color: AppPalette.textSecond
            font.pixelSize: Tokens.fontBase
        }

        KTabBar {
            width: parent.width
            fontPixelSize: Tokens.fontLg
            options: page.store ? page.store.autopilotCardScales.map(function(v) {
                return { label: Math.round(v * 100) + "%", value: v }
            }) : []
            currentValue: page.store ? page.store.autopilotCardScale : 1.0
            onValueSelected: function(v) { if (page.store) page.store.setAutopilotCardScale(v) }
        }
    }

    KIsland {
        KIslandRow {
            id: opacityRow
            label: qsTr("Opacity")
            slotWidth: Math.round(200 * AppPalette.scale)

            Item {
                width: opacityRow.slotWidth
                height: Tokens.controlHMd

                KSlider {
                    id: opacitySlider
                    anchors.left: parent.left
                    anchors.right: opacityValue.left
                    anchors.rightMargin: Tokens.spaceSm
                    anchors.verticalCenter: parent.verticalCenter
                    from: page.store ? page.store.autopilotCardOpacityMin : 0.3
                    to: 1
                    stepSize: 0.05
                    value: page.store ? page.store.autopilotCardOpacity : 1
                    onValueModified: function(val) { if (page.store) page.store.setAutopilotCardOpacity(val) }
                }
                Text {
                    id: opacityValue
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.round(44 * AppPalette.scale)
                    text: Math.round(opacitySlider.value * 100) + " %"
                    color: AppPalette.textStrong
                    font.pixelSize: Tokens.fontBase
                    horizontalAlignment: Text.AlignRight
                }
            }
        }
    }

    KButton {
        width: parent.width
        fontPixelSize: Tokens.fontLg
        text: qsTr("Defaults")
        toolTipText: qsTr("Show every button, 100 % size, fully opaque")
        onClicked: if (page.store) page.store.resetAutopilotCard()
    }
}
