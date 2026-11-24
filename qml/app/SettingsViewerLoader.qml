import QtQuick 2.15

Loader {
    id: root

    required property SettingsGroupBase group
    property var store: null
    readonly property bool panelOpen: !!(root.store && root.store.settingsPanelOpen === true)
    readonly property bool wanted: root.group.bodyShown && root.panelOpen
    property bool _keepAlive: false

    active: wanted || _keepAlive
    asynchronous: true

    onItemChanged: {
        if (!item)
            return
        root._keepAlive = true
        item.visible = Qt.binding(function() { return root.wanted || root.group.bodyRevealed })
        if ("shown" in item)
            item.shown = Qt.binding(function() { return root.wanted })
        if ("store" in item)
            item.store = Qt.binding(function() { return root.store })
    }
}
