import QtQuick

Item {
    id: root

    required property string key
    property Item target: parent

    property bool _ready: false
    property string _registeredKey: ""
    property Item _registeredTarget: null

    visible: false
    width: 0
    height: 0

    function _registry() {
        return (typeof tutorial !== "undefined" && tutorial) ? tutorial : null
    }

    function _register() {
        const registry = root._registry()
        if (!registry)
            return
        if (root._registeredKey !== "")
            registry.unregisterAnchor(root._registeredKey, root._registeredTarget)
        root._registeredKey = root.target ? root.key : ""
        root._registeredTarget = root.target
        if (root._registeredKey !== "")
            registry.registerAnchor(root._registeredKey, root._registeredTarget)
    }

    onKeyChanged: if (root._ready) root._register()
    onTargetChanged: if (root._ready) root._register()

    Component.onCompleted: {
        root._ready = true
        root._register()
    }

    Component.onDestruction: {
        const registry = root._registry()
        if (registry && root._registeredKey !== "")
            registry.unregisterAnchor(root._registeredKey, root._registeredTarget)
    }
}
