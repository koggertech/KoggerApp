import QtQuick 2.15
import kqml_types 1.0

BasePanePopup {
    id: root

    required property var store
    required property var def
    property real widgetScale: 1.0

    readonly property var _ds: (typeof dataset !== "undefined") ? dataset : null
    readonly property var _dmw: (typeof deviceManagerWrapper !== "undefined") ? deviceManagerWrapper : null

    readonly property real _bgAlpha: {
        var t = (def && typeof def.transparency === "number") ? def.transparency : 0
        return Math.max(0, Math.min(1, 1 - t / 100))
    }

    readonly property real _appScale: AppPalette.appScale
    readonly property real _k: _appScale * widgetScale

    readonly property int _cols: def ? def.cols : 1
    readonly property int _rows: def ? def.rows : 1

    readonly property real _cellW: Math.round(84 * _k)
    readonly property real _cellH: Math.round(84 * _k)
    readonly property real _gap:   Math.round(4 * _k)
    readonly property real _pad:   Math.round(8 * _k)

    readonly property real _gridW: _cols * _cellW + (_cols - 1) * _gap
    readonly property real _gridH: _rows * _cellH + (_rows - 1) * _gap

    readonly property real _baseCellW: Math.round(84 * _appScale)
    readonly property real _baseCellH: Math.round(84 * _appScale)
    readonly property real _baseGap:   Math.round(4 * _appScale)
    readonly property real _basePad:   Math.round(8 * _appScale)
    readonly property real _baseW: _cols * _baseCellW + (_cols - 1) * _baseGap + _basePad * 2 + contentPadding * 2
    readonly property real _baseH: _rows * _baseCellH + (_rows - 1) * _baseGap + _basePad * 2 + contentPadding * 2

    readonly property bool _beingEdited: !!(store && store.widgetEditorActive
        && store.widgetEditIndex >= 0 && store.widgetEditIndex < store.widgets.length
        && store.widgets[store.widgetEditIndex] && def
        && store.widgets[store.widgetEditIndex].id === def.id)

    popupVisible: !_beingEdited
    dragEnabled: true
    dragAnywhere: true
    headerReserved: false
    resizeEnabled: false
    collapseButtonVisible: false
    fullscreenMode: false
    panelColor: "transparent"
    panelBorderColor: "transparent"
    panelRadius: Tokens.radiusLg
    ghostFollowsContent: true
    ghostRadius: Tokens.radiusLg
    headerDragBarLength: 0
    snapEdgeCenters: true

    readonly property var _scaleSteps: [0.75, 1.0, 1.25, 1.5, 2.0, 2.5]
    resizeSnapSizes: {
        var out = []
        for (var i = 0; i < _scaleSteps.length; ++i)
            out.push(Qt.size(Math.round(_baseW * _scaleSteps[i]), Math.round(_baseH * _scaleSteps[i])))
        return out
    }

    function _applyScale() {
        expandedWidth  = Math.round(_gridW + _pad * 2 + contentPadding * 2)
        expandedHeight = Math.round(_gridH + _pad * 2 + contentPadding * 2)
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
    on_GridWChanged: _applyScale()
    on_GridHChanged: _applyScale()
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

    onSizeCommitted: function(w, h) {
        if (!def || !def.id)
            return
        widgetScale = Math.max(0.75, Math.min(2.5, w / _baseW))
        store.setWidgetScale(def.id, widgetScale)
    }

    dockState: (store && def && def.id) ? store.popupDock(popupId) : null
    onDockCommitted: function(targetId, side, gap, crossOffset) {
        store.setPopupDock(popupId, { targetId: targetId, side: side, gap: gap, cross: crossOffset })
    }

    Rectangle {
        anchors.fill: parent
        radius: root.panelRadius
        color: Qt.rgba(AppPalette.bg.r, AppPalette.bg.g, AppPalette.bg.b, root._bgAlpha)
        border.width: 0

        Item {
            width: root._gridW
            height: root._gridH
            anchors.centerIn: parent

            Repeater {
                model: root.def ? root.def.cells : []
                delegate: Item {
                    id: cell
                    required property var modelData
                    readonly property int _span: modelData.rep === "labelValueRow" ? 2 : 1
                    readonly property string _field: modelData.field
                    readonly property string _rep: modelData.rep
                    readonly property string _label: DataFieldCatalog.label(_field)
                    readonly property string _value: DataFieldCatalog.formatValue(_field, root._ds, root._dmw, root.store)

                    x: modelData.col * (root._cellW + root._gap)
                    y: modelData.row * (root._cellH + root._gap)
                    width: _span * root._cellW + (_span - 1) * root._gap
                    height: root._cellH

                    Text {
                        visible: cell._rep === "value"
                        anchors.fill: parent
                        anchors.margins: Math.round(6 * root._k)
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        fontSizeMode: Text.Fit
                        minimumPixelSize: Math.max(6, Math.round(7 * root._k))
                        font.pixelSize: Math.round(root._cellH * 0.6)
                        font.bold: true
                        elide: Text.ElideRight
                        text: cell._value
                        color: AppPalette.textStrong
                    }

                    Item {
                        visible: cell._rep === "labelValueStacked"
                        anchors.fill: parent
                        anchors.margins: Math.round(6 * root._k)

                        Text {
                            id: stLabel
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            height: parent.height * 0.45
                            verticalAlignment: Text.AlignVCenter
                            wrapMode: Text.WordWrap
                            maximumLineCount: 2
                            fontSizeMode: Text.Fit
                            minimumPixelSize: Math.max(6, Math.round(7 * root._k))
                            font.pixelSize: Math.round(root._cellH * 0.42)
                            font.bold: true
                            elide: Text.ElideRight
                            text: cell._label
                            color: AppPalette.textSecond
                        }
                        Text {
                            anchors.top: stLabel.bottom
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            verticalAlignment: Text.AlignVCenter
                            fontSizeMode: Text.Fit
                            minimumPixelSize: Math.max(6, Math.round(7 * root._k))
                            font.pixelSize: Math.round(root._cellH * 0.55)
                            font.bold: true
                            elide: Text.ElideRight
                            text: cell._value
                            color: AppPalette.textStrong
                        }
                    }

                    Item {
                        visible: cell._rep === "labelValueRow"
                        anchors.fill: parent
                        anchors.margins: Math.round(6 * root._k)

                        Text {
                            id: rowLabel
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            width: (parent.width - root._gap) / 2
                            verticalAlignment: Text.AlignVCenter
                            wrapMode: Text.WordWrap
                            maximumLineCount: 2
                            fontSizeMode: Text.Fit
                            minimumPixelSize: Math.max(6, Math.round(7 * root._k))
                            font.pixelSize: Math.round(root._cellH * 0.42)
                            font.bold: true
                            elide: Text.ElideRight
                            text: cell._label
                            color: AppPalette.textSecond
                        }
                        Text {
                            anchors.left: rowLabel.right
                            anchors.leftMargin: root._gap
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            verticalAlignment: Text.AlignVCenter
                            fontSizeMode: Text.Fit
                            minimumPixelSize: Math.max(6, Math.round(7 * root._k))
                            font.pixelSize: Math.round(root._cellH * 0.5)
                            font.bold: true
                            elide: Text.ElideRight
                            text: cell._value
                            color: AppPalette.textStrong
                        }
                    }
                }
            }
        }
    }
}
