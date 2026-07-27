import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import kqml_types 1.0

Column {
    id: root

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg
    readonly property real groupWidth: Math.max(0, width)

    Item {
        id: devicesReveal
        readonly property bool _hasDevices: deviceTopology.groups.length > 0
        width: root.groupWidth
        clip: true
        height: _hasDevices ? devicesContent.implicitHeight : 0
        visible: height > 0.5
        opacity: _hasDevices ? 1 : 0
        onVisibleChanged: if (visible) Qt.callLater(root._scrollToTop)

        Behavior on height  { NumberAnimation { duration: Anim.disclosureMs; easing.type: Anim.disclosureEasing } }
        Behavior on opacity { NumberAnimation { duration: Anim.fadeMs } }

        Column {
            id: devicesContent
            width: parent.width
            spacing: Tokens.spaceSm

            DeviceTopologyView {
                width: parent.width
                groups: deviceTopology.groups
                activeDevice: null
                onDeviceClicked: function(device) {
                    if (root.store) {
                        root.store.selectDevice(device)
                        root.store.openDeviceSettings()
                    }
                }
            }
        }
    }

    SettingsGroup {
        id: connGroup
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Connections")
        description: qsTr("Connections and logging.")
        stateStore: root.store
        stateKey: "app.connections"
        collapsedByDefault: false
        scrollIntoViewOnExpand: false
        contentSpacing: Tokens.spaceMd

        headerActions: [
            KCircleIconButton {
                readonly property bool _rec: typeof core !== "undefined" && core && (core.loggingKlf || core.loggingCsv)
                width: connGroup.headerActionSize
                height: connGroup.headerActionSize
                cornerRadius: Tokens.radiusLg   // uniform rounded chip, full header height
                borderWidth: 0
                scaleOnHover: false
                glyph: "REC"
                glyphPixelSize: Math.round(width * 0.42)
                glyphColor: _rec ? "#FFFFFF" : AppPalette.text
                toolTipText: _rec ? qsTr("Stop recording") : qsTr("Start recording")
                fillColor:      _rec ? "#7F1D1D" : AppPalette.chipRaised
                fillHoverColor: _rec ? "#991B1B" : AppPalette.chipRaisedHover
                onClicked: if (root.store) root.store.setRecording(!root.store.isRecording)
            },
            KCircleIconButton {
                id: reopenBtn
                readonly property var _info: root.store ? root.store.reconnectInfo : null
                readonly property int _state: reopenBtn._info ? reopenBtn._info.worst : 0
                readonly property int _count: reopenBtn._info ? reopenBtn._info.count : 0
                readonly property bool _allOpen: !!(reopenBtn._info && reopenBtn._info.allOpen)

                height: connGroup.headerActionSize
                width: reopenBtn._count > 0 ? connGroup.headerActionSize : 0
                opacity: reopenBtn._count > 0 ? 1 : 0
                visible: opacity > 0.01
                clip: true
                Behavior on width   { NumberAnimation { duration: Anim.fadeMs; easing.type: Easing.OutCubic } }
                Behavior on opacity { NumberAnimation { duration: Anim.fadeMs } }

                cornerRadius: Tokens.radiusLg
                borderWidth: 0
                scaleOnHover: false
                iconSource: "qrc:/icons/ui/plug.svg"
                iconTintColor: AppPalette.text
                iconPixelSize: Math.round(connGroup.headerActionSize * 0.5)
                toolTipText: reopenBtn._allOpen ? qsTr("Disconnect") : qsTr("Open last link")
                fillColor:      reopenBtn._state === 1 ? AppPalette.linkOkBg : reopenBtn._state === 2 ? AppPalette.linkIdleBg : AppPalette.chipRaised
                fillHoverColor: reopenBtn._state === 1 ? AppPalette.linkOkBg : reopenBtn._state === 2 ? AppPalette.linkIdleBg : AppPalette.chipRaisedHover
                onClicked: if (root.store) root.store.toggleRememberedLinks()

                Text {
                    visible: reopenBtn._count > 1
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.rightMargin: Math.round(3 * AppPalette.scale)
                    anchors.bottomMargin: Math.round(2 * AppPalette.scale)
                    text: reopenBtn._count
                    color: AppPalette.text
                    font.pixelSize: Math.round(9 * AppPalette.scale)
                    font.bold: true
                    style: Text.Outline
                    styleColor: "#000000B0"
                }
            }
        ]

        Loader {
            id: connectionsLoader
            width: parent.width
            active: connGroup.expanded
                    && (root.store ? root.store.settingsPanelOpen === true : false)
            asynchronous: true
            source: "qrc:/qml/devices/ConnectionViewer.qml"

            onLoaded: {
                if (item) {
                    item.width = width
                    item.store = root.store
                }
            }

            onWidthChanged: {
                if (item)
                    item.width = width
            }
        }
    }

    function _findAncestorFlickable() {
        var item = root.parent
        while (item) {
            if (item.contentY !== undefined
                    && item.contentHeight !== undefined
                    && item.contentWidth !== undefined)
                return item
            item = item.parent
        }
        return null
    }

    function _scrollToTop() {
        var flick = _findAncestorFlickable()
        if (!flick) return
        if (flick.contentY <= 0.5) return
        scrollToTopAnim.stop()
        scrollToTopAnim.target = flick
        scrollToTopAnim.from = flick.contentY
        scrollToTopAnim.to = 0
        scrollToTopAnim.start()
    }

    NumberAnimation {
        id: scrollToTopAnim
        property: "contentY"
        duration: 240
        easing.type: Easing.OutCubic
    }
}
