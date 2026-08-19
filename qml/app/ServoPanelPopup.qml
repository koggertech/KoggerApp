import QtQuick 2.15
import kqml_types 1.0

BasePanePopup {
    id: root

    required property var store
    required property var def

    property real widgetScale: 1.0

    readonly property real _bgAlpha: {
        var t = (def && typeof def.transparency === "number") ? def.transparency : 0
        return Math.max(0, Math.min(1, 1 - t / 100))
    }

    // The content is built out of real controls at their real token sizes, so the panel's own
    // scale is a transform on the finished layout rather than a factor threaded through every
    // metric. A KSpinBox rebuilt in _k-space would be a second copy of a control that already
    // exists, and it would drift from the one in device settings.
    readonly property real _k: widgetScale

    // The cap the content scrolls under. Measured against the workspace the panel floats in,
    // and in the body's own (unscaled) units, which is why _k divides out.
    readonly property real _maxContentH: Math.max(Math.round(160 * AppPalette.scale),
                                                  height * 0.9 - _contentTopMargin - contentPadding) / Math.max(0.01, _k)
    readonly property real _contentH: scroller.viewHeight

    popupVisible: true
    dragEnabled: true
    // Dragged by the title strip, not by the body: the body scrolls once "More settings" is
    // open, and a body DragHandler takes the grab off the Flickable, so a scroll gesture would
    // move the panel instead. Not by the header grip either -- the header band is off
    // (headerReserved) and the grip hidden (dragHandleOpacity), because a chrome row above a
    // stack of controls costs height the panel has none to spare.
    dragAnywhere: false
    headerReserved: false
    dragHandleOpacity: 0
    resizeEnabled: false
    collapseButtonVisible: false
    fullscreenMode: false
    panelColor: Qt.rgba(AppPalette.bg.r, AppPalette.bg.g, AppPalette.bg.b, _bgAlpha)
    panelBorderColor: AppPalette.border
    panelRadius: Tokens.radiusLg
    ghostFollowsContent: true
    ghostRadius: Tokens.radiusLg
    snapEdgeCenters: true

    // Imperative, like every other panel: BasePanePopup writes into expandedWidth/Height itself
    // on drag and snap, and a binding would be broken the first time it did.
    function _applyScale() {
        expandedWidth  = Math.round(scroller.bodyWidth * _k + contentPadding * 2)
        expandedHeight = Math.round(_contentH * _k + _contentTopMargin + contentPadding)
    }

    property bool _synced: false

    function syncFromStore() {
        if (!def || !def.id)
            return
        widgetScale = store.widgetScale(def.id)
        _applyScale()
        var p = store.widgetPosition(def.id, popupWidth, popupHeight)
        var rb = store.widgetRevealBounds(popupWidth, popupHeight)
        var nx = Math.max(rb.minX, Math.min(rb.maxX, p.x))
        var ny = Math.max(rb.minY, Math.min(rb.maxY, p.y))
        suspendSignals = true
        panelX = clampX(nx)
        panelY = clampY(ny)
        suspendSignals = false
        _synced = true
    }

    onWidgetScaleChanged: _applyScale()
    on_ContentHChanged: _applyScale()
    onDefChanged: { _applyScale(); Qt.callLater(syncFromStore) }

    Component.onCompleted: {
        syncFromStore()
        Qt.callLater(syncFromStore)
        Qt.callLater(resolveOverlapWithSibling)
    }

    onPositionCommitted: function(x, y, w, h) {
        if (_synced && def && def.id)
            store.setWidgetPosition(def.id, x, y, w, h)
    }

    onInteractionStarted: if (store && def && def.id) store.widgetBringToFront(def.id)

    dockState: (store && def && def.id) ? store.popupDock(popupId) : null
    onDockCommitted: function(targetId, side, gap, crossOffset) {
        store.setPopupDock(popupId, { targetId: targetId, side: side, gap: gap, cross: crossOffset })
    }

    ServoPanelScroll {
        id: scroller
        maxHeight: root._maxContentH
        transformOrigin: Item.TopLeft
        scale: root._k
    }

    // The panel's title strip, doubling as its drag handle: it costs no height because the
    // island's own title already sits there, and it keeps the grab away from the scrolling body.
    Item {
        width: parent.width
        height: Math.round(24 * AppPalette.scale) * root._k
        z: 10

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            cursorShape: Qt.OpenHandCursor
            onPressed: function(mouse) { mouse.accepted = true }
        }

        DragHandler {
            target: null
            enabled: root.dragEnabled && !root.collapsed && !root.fullscreenMode
            xAxis.enabled: true
            yAxis.enabled: true
            onActiveChanged: active ? root._beginDrag() : root._endDrag()
            onTranslationChanged: if (active) root._updateDrag(translation.x, translation.y)
        }
    }
}
