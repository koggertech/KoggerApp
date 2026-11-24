import QtQuick 2.15

QtObject {
    id: root

    property Item targetItem: null
    property string text: ""
    property bool shown: false
    property int delay: Anim.tooltipDelayMs
    readonly property bool tipCreated: root._tip !== null
    readonly property bool tipVisible: root._tip !== null && root._tip.visible

    property var _tip: null

    function _release() {
        const tip = root._tip
        if (tip === null)
            return
        root._tip = null
        tip.destroy()
    }

    function _open() {
        const tip = ToolTipFactory.toolTip.createObject(root)
        if (tip === null)
            return
        if (root.targetItem !== null)
            tip.parent = root.targetItem
        tip.targetItem = Qt.binding(function() { return root.targetItem })
        tip.text = Qt.binding(function() { return root.text })
        tip.delay = Qt.binding(function() { return root.delay })
        tip.closed.connect(root._release)
        root._tip = tip
        tip.shown = Qt.binding(function() { return root.shown })
    }

    function _sync() {
        if (root.shown && root.text.length > 0) {
            if (root._tip === null)
                root._open()
        } else if (root._tip !== null && !root._tip.visible) {
            root._release()
        }
    }

    onShownChanged: _sync()
    onTextChanged: _sync()
    Component.onCompleted: _sync()
}
