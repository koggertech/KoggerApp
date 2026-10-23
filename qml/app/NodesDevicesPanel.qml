import QtQuick 2.15
import QtQuick.Controls 2.15
import kqml_types 1.0
import "NodesModel.js" as Model

Item {
    id: root

    property var store: null
    property var writer: null
    property var engine: null
    property var workspaceStore: null

    signal quickInterrogateRequested(var dev)

    component Heading: Text {
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontXs; font.bold: true
        topPadding: Tokens.spaceSm
    }
    component Btn: UsblButton {
        implicitHeight: Tokens.controlHMd
        fontPixelSize: Tokens.fontSm
    }

    Flickable {
        id: flick
        anchors.fill: parent
        contentWidth: col.width + 2 * Tokens.spaceSm
        contentHeight: col.implicitHeight + 2 * Tokens.spaceSm
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: col
            x: Tokens.spaceSm; y: Tokens.spaceSm
            width: Math.max(flick.width - 2 * Tokens.spaceSm, Math.round(210 * AppPalette.scale))
            spacing: Tokens.spaceXs

            Heading { text: qsTr("LIFELINES") }
            Repeater {
                model: root.store ? root.store.lifelines : []
                delegate: Rectangle {
                    id: row
                    required property var modelData
                    readonly property var _dev: root.store.deviceFor(modelData.id)
                    readonly property string _ws: root.store.writeStateOf(modelData.id)
                    readonly property bool _sel: root.store.selKind === "lifeline" && root.store.selId === modelData.id
                    width: col.width
                    height: Math.round(34 * AppPalette.scale)
                    radius: Tokens.radiusSm
                    color: _sel ? AppPalette.accentBg : (rowTap.containsMouse ? AppPalette.cardHover : "transparent")
                    Row {
                        anchors.left: parent.left
                        anchors.leftMargin: Tokens.spaceXs
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: Tokens.spaceXs
                        Rectangle {
                            width: Math.round(8 * AppPalette.scale); height: width; radius: width / 2
                            anchors.verticalCenter: parent.verticalCenter
                            color: row._dev ? AppPalette.linkOkBorder : "transparent"
                            border.width: row._dev ? 0 : 1
                            border.color: row.modelData.binding ? AppPalette.linkIdleBorder : AppPalette.textMuted
                        }
                        UsblAddressBadge {
                            visible: typeof row.modelData.address === "number"
                            address: typeof row.modelData.address === "number" ? row.modelData.address : 0
                            diameter: Math.round(18 * AppPalette.scale)
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Column {
                            anchors.verticalCenter: parent.verticalCenter
                            Text {
                                text: row.modelData.label
                                color: AppPalette.textStrong; font.pixelSize: Tokens.fontXs; font.bold: true
                                width: col.width - Math.round(54 * AppPalette.scale); elide: Text.ElideRight
                            }
                            Text {
                                text: row._dev ? ("SN " + row._dev.devSN + " · " + root.store.linkLabelOf(row._dev))
                                     : row.modelData.binding ? qsTr("SN %1 · not connected").arg(row.modelData.binding.serial)
                                     : row.modelData.kind === "external" ? qsTr("external") : qsTr("unbound")
                                color: AppPalette.textMuted; font.pixelSize: Tokens.fontXxs
                                width: col.width - Math.round(54 * AppPalette.scale); elide: Text.ElideRight
                            }
                        }
                    }
                    MouseArea {
                        id: rowTap
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: root.store.select("lifeline", row.modelData.id)
                    }
                }
            }
            Flow {
                width: col.width
                spacing: Tokens.spaceXs
                Btn { text: qsTr("+ Head"); onClicked: root.store.addLifeline("head") }
                Btn { text: qsTr("+ Beacon"); onClicked: root.store.addLifeline("beacon") }
                Btn { text: qsTr("+ External"); toolTipText: qsTr("A party this app does not configure, e.g. another vessel's interrogator"); onClicked: root.store.addLifeline("external") }
            }

            Heading { text: qsTr("CONNECTED, NOT IN THIS NETWORK") }
            Text {
                visible: !root.store || !root.store.unboundDevices.length
                width: col.width; wrapMode: Text.WordWrap
                text: root.store && root.store.devices.length ? qsTr("Every connected USBL device is bound.")
                                                              : qsTr("No USBL device connected. Connect one below — or design now and bind later.")
                color: AppPalette.textMuted; font.pixelSize: Tokens.fontXs
            }
            Repeater {
                model: root.store ? root.store.unboundDevices : []
                delegate: Rectangle {
                    id: drow
                    required property var modelData
                    width: col.width
                    implicitHeight: dcol.implicitHeight + 2 * Tokens.spaceXs
                    radius: Tokens.radiusSm
                    color: AppPalette.card
                    border.width: 1
                    border.color: AppPalette.border
                    Column {
                        id: dcol
                        x: Tokens.spaceXs; y: Tokens.spaceXs
                        width: parent.width - 2 * Tokens.spaceXs
                        spacing: Tokens.spaceXxs
                        Text {
                            text: drow.modelData.devName + " SN " + drow.modelData.devSN
                            color: AppPalette.textStrong; font.pixelSize: Tokens.fontXs; font.bold: true
                            width: parent.width; elide: Text.ElideRight
                        }
                        Text {
                            text: qsTr("fw %1 · %2").arg(drow.modelData.fwVersion).arg(root.store.linkLabelOf(drow.modelData))
                            color: AppPalette.textMuted; font.pixelSize: Tokens.fontXxs
                            width: parent.width; elide: Text.ElideRight
                        }
                        Flow {
                            width: parent.width
                            spacing: Tokens.spaceXxs
                            Btn {
                                readonly property var _l: root.store.selLifeline
                                visible: !!_l && _l.kind !== "external"
                                text: _l ? qsTr("Bind to %1").arg(_l.label) : ""
                                onClicked: root.store.bindLifeline(_l.id, drow.modelData)
                            }
                            Btn {
                                text: qsTr("Add as head")
                                onClicked: { var id = root.store.addLifeline("head"); root.store.bindLifeline(id, drow.modelData) }
                            }
                            Btn {
                                text: qsTr("Add as beacon")
                                onClicked: { var id = root.store.addLifeline("beacon"); root.store.bindLifeline(id, drow.modelData) }
                            }
                            Btn { text: qsTr("Quick…"); toolTipText: qsTr("Interrogate addresses from this device now"); onClicked: root.quickInterrogateRequested(drow.modelData) }
                        }
                    }
                }
            }

            Heading { text: qsTr("LINKS") }
            Row {
                spacing: Tokens.spaceXs
                readonly property var _info: root.workspaceStore ? root.workspaceStore.reconnectInfo : null
                Btn {
                    enabled: !!parent._info && parent._info.count > 0
                    text: parent._info && parent._info.allOpen ? qsTr("Disconnect links") : qsTr("Connect recent links")
                    toolTipText: parent._info ? qsTr("%1 recently used links, %2 open").arg(parent._info.count).arg(parent._info.open) : ""
                    onClicked: root.workspaceStore.toggleRememberedLinks()
                }
                Text {
                    text: parent._info ? qsTr("%1 / %2 open").arg(parent._info.open).arg(parent._info.count) : ""
                    color: AppPalette.textMuted; font.pixelSize: Tokens.fontXxs
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            Text {
                width: col.width; wrapMode: Text.WordWrap
                text: qsTr("A device plugged in on a known port connects here; a new port is added once in the connections settings.")
                color: AppPalette.textMuted; font.pixelSize: Tokens.fontXxs
            }

            Heading { text: qsTr("UPLOAD QUEUE") }
            Repeater {
                model: root.store ? root.store.lifelines.filter(function (l) { return l.kind !== "external" && !!l.binding }) : []
                delegate: Row {
                    id: qrow
                    required property var modelData
                    readonly property var _dev: root.store.deviceFor(modelData.id)
                    readonly property string _ws: root.store.writeStateOf(modelData.id)
                    spacing: Tokens.spaceXs
                    width: col.width
                    Text {
                        text: (qrow._ws === "confirmed" ? "✓ " : qrow._ws === "writing" ? "… " : root.store.needsWrite(qrow.modelData.id) ? "• " : "  ") + qrow.modelData.label
                        color: qrow._ws === "confirmed" ? AppPalette.linkOkText
                             : (qrow._ws === "rejected" || qrow._ws === "noAnswer" || qrow._ws === "partial") ? AppPalette.dangerText
                             : AppPalette.text
                        font.pixelSize: Tokens.fontXs
                        width: col.width - writeBtn.width - Tokens.spaceXs
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                        KToolTip { text: root.store.writeStateText(qrow._ws); shown: qtip.hovered }
                        HoverHandler { id: qtip }
                    }
                    Btn {
                        id: writeBtn
                        text: qrow._ws === "writing" && root.writer ? (root.writer.progressDone + "/" + root.writer.progressTotal) : qsTr("Write")
                        enabled: !!qrow._dev && !!root.writer && !root.writer.busy
                        onClicked: root.writer.writeLifeline(qrow.modelData.id)
                    }
                }
            }
            Text {
                visible: !!root.store && root.store.lifelines.some(function (l) { return l.kind === "beacon" && !!l.binding && !root.store.deviceFor(l.id) && root.store.writeStateOf(l.id) === "earlier" })
                width: col.width; wrapMode: Text.WordWrap
                text: qsTr("Written beacons keep their settings only while powered.")
                color: AppPalette.linkIdleText; font.pixelSize: Tokens.fontXxs
            }
        }
    }
    KScrollBar {
        flickable: flick
        anchors.top: flick.top
        anchors.bottom: flick.bottom
        anchors.right: flick.right
    }
    KScrollBar {
        flickable: flick
        orientation: Qt.Horizontal
        anchors.left: flick.left
        anchors.right: flick.right
        anchors.bottom: flick.bottom
    }
}
