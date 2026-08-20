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
    readonly property real _barH: headerHeight
    readonly property real _barPad: contentPadding
    readonly property real _barBtn: Math.max(Math.round(18 * AppPalette.scale), _barH - _barPad * 2)

    readonly property real _maxContentH: Math.max(Math.round(160 * AppPalette.scale),
                                                  height * 0.9 - _contentTopMargin - contentPadding) / Math.max(0.01, _k)
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
        expandedWidth  = Math.round(scroller.bodyWidth * _k + contentPadding * 2)
        expandedHeight = Math.round(_contentH * _k + _contentTopMargin + contentPadding)
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

    ServoPanelScroll {
        id: scroller
        active: root.popupVisible
        maxHeight: root._maxContentH
        opacity: root._bgAlpha
        transformOrigin: Item.TopLeft
        scale: root._k
    }

    Item {
        id: titleBar
        width: parent.width
        height: root._barH
        y: -root._barH
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

        Text {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Servo")
            color: AppPalette.textStrong
            font.pixelSize: Tokens.fontBase
        }

        KDragBar {
            anchors.centerIn: parent
            orientation: "horizontal"
            barColor: AppPalette.controlRaised
        }

        KCircleIconButton {
            id: closeBtn
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: root._barPad
            width: root._barBtn
            height: root._barBtn
            rounded: false
            cornerRadius: Tokens.radiusLg
            z: 1
            iconSource: ""
            glyph: "×"
            showGlyphWithIcon: true
            glyphPixelSize: Tokens.iconSm
            glyphColor: AppPalette.textSecond
            fillColor: AppPalette.controlRaised
            fillHoverColor: Qt.lighter(AppPalette.controlRaised, 1.2)
            fillPressedColor: AppPalette.bgDeep
            borderColor: AppPalette.border
            borderHoverColor: AppPalette.borderHover
            toolTipText: qsTr("Hide panel")
            onClicked: root.store.setServoPanelShown(false)
        }

        KCircleIconButton {
            anchors.right: closeBtn.left
            anchors.rightMargin: root._barPad
            anchors.top: closeBtn.top
            width: root._barBtn
            height: root._barBtn
            rounded: false
            cornerRadius: Tokens.radiusLg
            z: 1
            iconSource: "qrc:/icons/ui/pencil.svg"
            iconTintColor: AppPalette.textSecond
            fillColor: AppPalette.controlRaised
            fillHoverColor: Qt.lighter(AppPalette.controlRaised, 1.2)
            fillPressedColor: AppPalette.bgDeep
            borderColor: AppPalette.border
            borderHoverColor: AppPalette.borderHover
            toolTipText: qsTr("Panel settings")
            onClicked: root.store.openServoPanelSettings()
        }
    }
}
