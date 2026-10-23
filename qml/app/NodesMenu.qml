import QtQuick 2.15
import QtQuick.Controls 2.15
import kqml_types 1.0

// A context menu drawn like the settings pages: islands of interactive rows, sub-menus as
// sub-pages with a back row, toggles as flat switches.
Popup {
    id: menu

    /**
     * Groups of items, one island per group:
     * [[{ text, enabled, visible, danger, checked, tip, elideLeft, onTriggered, items }]].
     * `checked` (a bool) adds a switch; `items` (groups, or a function returning them) opens a sub-page;
     * `elideLeft` keeps the end of a long text, such as a file path.
     */
    property var groups: []

    property var _stack: []
    property real _px: 0
    property real _py: 0
    readonly property var _page: _stack.length ? _stack[_stack.length - 1] : { title: "", groups: groups }
    readonly property var _shownGroups: {
        var out = []
        var gs = _page.groups || []
        for (var i = 0; i < gs.length; ++i) {
            var g = (gs[i] || []).filter(function (it) { return it && it.visible !== false })
            if (g.length) out.push(g)
        }
        return out
    }
    readonly property real _margin: Tokens.spaceSm
    readonly property real _rowPad: Tokens.spaceLg
    readonly property real _contentW: {
        var w = _stack.length ? fm.advanceWidth("‹ " + _page.title) : 0
        for (var i = 0; i < _shownGroups.length; ++i)
            for (var k = 0; k < _shownGroups[i].length; ++k) {
                var it = _shownGroups[i][k]
                var extra = typeof it.checked === "boolean" ? Math.round(52 * AppPalette.scale)
                          : it.items ? Math.round(24 * AppPalette.scale) : 0
                w = Math.max(w, fm.advanceWidth(String(it.text || "")) + extra)
            }
        return w + 2 * _rowPad + Tokens.spaceMd
    }

    /** Opens the menu with its top-left corner at a point in window coordinates. */
    function openAt(sceneX, sceneY) {
        _stack = []
        _px = sceneX
        _py = sceneY
        open()
    }

    function _activate(it) {
        if (!it || it.enabled === false) return
        if (it.items) {
            var gs = typeof it.items === "function" ? it.items() : it.items
            _stack = _stack.concat([{ title: String(it.text || ""), groups: gs }])
            return
        }
        close()
        if (typeof it.onTriggered === "function") it.onTriggered()
    }
    function _back() { _stack = _stack.slice(0, _stack.length - 1) }

    parent: Overlay.overlay
    x: Math.max(_margin, Math.min(_px, (parent ? parent.width : 0) - width - _margin))
    y: Math.max(_margin, Math.min(_py, (parent ? parent.height : 0) - height - _margin))
    width: Math.min(Math.max(_contentW + 2 * padding, Math.round(200 * AppPalette.scale)),
                    Math.round(440 * AppPalette.scale))
    padding: Tokens.spaceSm
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    onClosed: _stack = []

    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Anim.tooltipMs } }
    exit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Anim.tooltipExitMs } }

    FontMetrics { id: fm; font.pixelSize: Tokens.fontBase }

    background: Rectangle {
        color: AppPalette.bg
        radius: Tokens.radiusLg
        border.width: 1
        border.color: AppPalette.border
    }

    contentItem: Column {
        spacing: Tokens.spaceSm

        KIsland {
            visible: menu._stack.length > 0
            width: parent.width
            labelPixelSize: Tokens.fontBase
            KIslandRow {
                label: "‹ " + menu._page.title
                labelBold: true
                interactive: true
                onClicked: menu._back()
            }
        }

        Repeater {
            model: menu._shownGroups
            delegate: KIsland {
                required property var modelData
                width: parent ? parent.width : 0
                labelPixelSize: Tokens.fontBase

                Repeater {
                    model: modelData
                    delegate: KIslandRow {
                        id: row
                        required property var modelData
                        readonly property bool _on: modelData.enabled !== false
                        label: String(modelData.text || "")
                        labelElide: modelData.elideLeft ? Text.ElideLeft : Text.ElideRight
                        labelColor: modelData.danger ? AppPalette.dangerText : AppPalette.textStrong
                        interactive: _on
                        enabled: _on
                        opacity: _on ? 1.0 : 0.45
                        chevron: !!modelData.items
                        toolTipText: modelData.tip || ""
                        onClicked: menu._activate(modelData)

                        Loader {
                            active: typeof row.modelData.checked === "boolean"
                            sourceComponent: KSwitch {
                                flat: true
                                enabled: row._on
                                checked: !!row.modelData.checked
                                onToggled: menu._activate(row.modelData)
                            }
                        }
                    }
                }
            }
        }
    }
}
