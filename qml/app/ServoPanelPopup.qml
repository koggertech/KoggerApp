import QtQuick 2.15
import kqml_types 1.0

BasePanePopup {
    id: root

    required property var store
    required property var def

    property real widgetScale: 1.0

    readonly property real _bgAlpha: {
        var t = store ? store.servoPanelTransparency : 0
        return Math.max(0.15, Math.min(1, 1 - t / 100))
    }

    readonly property real _k: widgetScale
    readonly property real _inset: Math.round(Tokens.spaceMd * _k)

    readonly property real _maxContentH: Math.max(Math.round(160 * AppPalette.scale),
                                                  height * 0.9 - _contentTopMargin - contentPadding - _inset * 2) / Math.max(0.01, _k)
    readonly property real _contentH: scroller.viewHeight

    popupVisible: true
    dragEnabled: true
    dragAnywhere: false
    headerReserved: true
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

    function _applyScale() {
        expandedWidth  = Math.round(scroller.bodyWidth * _k + _inset * 2 + contentPadding * 2)
        expandedHeight = Math.round(_contentH * _k + _inset * 2 + _contentTopMargin + contentPadding)
    }

    property bool _synced: false

    function syncFromStore() {
        if (!def || !def.id)
            return
        widgetScale = store.widgetScale(def.id)
        _applyScale()
        var p = store.servoPanelPosition(popupWidth, popupHeight)
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

    headerContent: PanelTitleBar {
        popup: root
        title: qsTr("Servo")
        titleInset: root.contentPadding + root._inset
        onSettingsClicked: root.store.openServoPanelSettings()
        onCloseClicked: root.store.setServoPanelShown(false)
    }

    ServoPanelScroll {
        id: scroller
        x: root._inset
        y: root._inset
        active: root.popupVisible
        maxHeight: root._maxContentH
        opacity: root._bgAlpha
        transformOrigin: Item.TopLeft
        scale: root._k
    }
}
