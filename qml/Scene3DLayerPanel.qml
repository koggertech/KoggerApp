import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtCore

Item {
    id: root
    anchors.right: parent.right
    anchors.top: parent.top
    anchors.bottom: parent.bottom

    signal requestClose()

    property bool expanded: false
    property real panelWidth: theme.controlHeight * 6
    property real panelPadding: 10
    property bool showToggleButton: true

    width: (showToggleButton ? toggleButton.implicitWidth : 0) + panel.width + panelPadding

    function formatSectionLabel(text) {
        if (!text || text.length === 0) {
            return ""
        }
        return text.charAt(0).toUpperCase() + text.slice(1)
    }

    CheckButton {
        id: toggleButton
        iconSource: "qrc:/icons/ui/map.svg"
        backColor: theme.controlBackColor
        borderColor: theme.controlBackColor
        checkedBorderColor: theme.controlBorderColor
        checkable: false
        checked: false
        implicitHeight: theme.controlHeight * 1.3
        implicitWidth: theme.controlHeight * 1.3

        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 8
        anchors.rightMargin: 8
        visible: showToggleButton

        CMouseOpacityArea {
            toolTipText: qsTr("Map settings")
            popupPosition: "topRight"
        }

        onClicked: {
            if (showToggleButton) {
                root.expanded = !root.expanded
            }
        }
    }

    Rectangle {
        id: panel
        width: root.expanded ? root.panelWidth : 0
        anchors.right: showToggleButton ? toggleButton.left : parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.rightMargin: showToggleButton ? 6 : 0
        color: theme.menuBackColor
        border.color: theme.controlBorderColor
        border.width: 1
        radius: 4
        clip: true
        enabled: root.expanded
        visible: root.expanded || width > 0
        opacity: root.expanded ? 1.0 : 0.0

        Behavior on width { NumberAnimation { duration: 160; easing.type: Easing.OutQuad } }
        Behavior on opacity { NumberAnimation { duration: 120 } }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: root.panelPadding
            spacing: 8

            Text {
                text: qsTr("Map settings")
                color: theme.textColor
                font: theme.textFont
                Layout.fillWidth: true
            }

            CheckButton {
                id: mapViewCheckButton
                objectName: "mapViewCheckButton"
                backColor: theme.controlBackColor
                borderColor: theme.controlBackColor
                checkedBorderColor: theme.controlBorderColor
                checked: true
                //iconSource: "qrc:/icons/ui/map.svg"
                text: qsTr("Visibility")
                Layout.fillWidth: true

                onToggled: {
                    MapViewControlMenuController.onVisibilityChanged(checked)
                }

                Component.onCompleted: {
                    MapViewControlMenuController.onVisibilityChanged(checked)
                }

                Settings {
                    property alias mapViewCheckButton: mapViewCheckButton.checked
                }
            }

            Text {
                text: qsTr("Providers")
                color: theme.textColor
                font: theme.textFont
                opacity: 0.7
                Layout.fillWidth: true
            }

            ListView {
                id: providerList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 4
                model: core.mapTileProviders

                section.property: "layer_type"
                section.criteria: ViewSection.FullString
                //section.delegate: Text {
                //    text: root.formatSectionLabel(section)
                //    color: theme.textColor
                //    font: theme.textFont
                //    opacity: 0.7
                //}

                delegate: CheckButton {
                    width: parent ? parent.width : panel.width
                    text: modelData.name
                    checkable: true
                    checked: modelData.id === core.mapTileProviderId
                    backColor: theme.controlBackColor
                    borderColor: theme.controlBorderColor
                    checkedBorderColor: theme.controlBorderColor

                    onClicked: {
                        core.setMapTileProvider(modelData.id)
                        if (showToggleButton) {
                            root.expanded = false
                        } else {
                            root.requestClose()
                        }
                    }
                }
            }
        }
    }
}
