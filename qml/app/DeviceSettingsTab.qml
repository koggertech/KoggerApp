import QtQuick 2.15
import kqml_types 1.0

Item {
    id: page

    required property var store

    width: parent ? parent.width : implicitWidth
    implicitHeight: topoWrap.height + Tokens.spaceLg + contentCol.implicitHeight

    property var _flick: null
    function _findFlick() {
        var it = page.parent
        while (it) {
            if (it.contentY !== undefined && it.contentHeight !== undefined && it.flickableDirection !== undefined)
                return it
            it = it.parent
        }
        return null
    }
    Component.onCompleted: _flick = _findFlick()

    property var activeDev: store ? store.activeDevice : null
    property var _scrollByDev: []
    property var _prevDev: null

    function _scrollEntry(d) {
        for (var i = 0; i < _scrollByDev.length; ++i)
            if (_scrollByDev[i].dev === d) return _scrollByDev[i]
        return null
    }

    onActiveDevChanged: {
        if (_prevDev && _flick) {
            var e = _scrollEntry(_prevDev)
            if (e) e.y = _flick.contentY
            else _scrollByDev.push({ dev: _prevDev, y: _flick.contentY })
        }
        _prevDev = activeDev
        restoreScrollTimer.restart()
    }

    Timer {
        id: restoreScrollTimer
        interval: 50
        onTriggered: {
            if (!page._flick) return
            var e = page._scrollEntry(page.activeDev)
            var y = e ? e.y : 0
            var maxY = Math.max(0, page._flick.contentHeight - page._flick.height)
            page._flick.contentY = Math.max(0, Math.min(y, maxY))
        }
    }

    Column {
        id: contentCol
        anchors.left: parent.left
        anchors.right: parent.right
        y: topoWrap.height + Tokens.spaceLg
        spacing: Tokens.spaceLg

        Text {
            visible: !!(page.store && page.store.activeDevice)
            text: qsTr("Settings:")
            color: AppPalette.textSecond
            font.pixelSize: Tokens.fontBase
            leftPadding: Tokens.spaceXxs
        }

        DeviceSettingsPage {
            width: parent.width
            visible: !!(page.store && page.store.activeDevice)
            dev: page.store ? page.store.activeDevice : null
            store: page.store
        }
    }

    Rectangle {
        z: 9
        width: parent.width
        y: topoWrap.y + topoWrap.height
        height: Tokens.spaceMd
        visible: !!(page._flick && page._flick.contentY > 0.5)
        gradient: Gradient {
            GradientStop { position: 0.0; color: AppPalette.bg }
            GradientStop { position: 1.0; color: Qt.rgba(AppPalette.bg.r, AppPalette.bg.g, AppPalette.bg.b, 0) }
        }
    }

    Item {
        id: topoWrap
        width: parent.width
        height: topo.implicitHeight
        z: 10
        y: page._flick ? Math.max(0, page._flick.contentY) : 0

        Rectangle { anchors.fill: parent; color: AppPalette.bg }

        DeviceTopologyView {
            id: topo
            width: parent.width
            groups: (typeof deviceTopology !== "undefined" && deviceTopology) ? deviceTopology.groups : []
            activeDevice: page.store ? page.store.activeDevice : null
            onDeviceClicked: function(device) { if (page.store) page.store.selectDevice(device) }
        }
    }
}
