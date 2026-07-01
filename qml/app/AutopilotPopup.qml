import QtQuick 2.15
import kqml_types 1.0

BasePanePopup {
    id: root

    required property var store

    readonly property real _s: 1.5 * (theme ? theme.resCoeff : 1.0)
    readonly property int _pad: Math.round(6 * _s)
    readonly property int _cellGap: Math.round(11 * _s)

    readonly property var _dmw: (typeof deviceManagerWrapper !== "undefined") ? deviceManagerWrapper : null
    readonly property real _voltage: _dmw ? _dmw.vruVoltage : NaN
    readonly property real _current: _dmw ? _dmw.vruCurrent : NaN
    readonly property real _speed:   _dmw ? _dmw.vruVelocityH : NaN
    readonly property int  _arm:     _dmw ? _dmw.pilotArmState : -1
    readonly property int  _mode:    _dmw ? _dmw.pilotModeState : -1

    readonly property int _controlH: Math.round(36 * _s) - 2
    readonly property int _sidePad: Math.round(3 * _s)
    readonly property int _panelRadius: Math.round((_controlH + _sidePad * 2) / 2)
    readonly property real _cardW: Math.round(_pad + infoRow.implicitWidth + _cellGap + _controlH + _sidePad)
    readonly property real _cardH: Math.round(_controlH + _sidePad * 2)
    readonly property real _wantW: _cardW + contentPadding * 2
    readonly property real _wantH: headerHeight + _cardH + contentPadding

    popupVisible: store.autopilotEnabled
    dragEnabled: true
    resizeEnabled: false
    collapseButtonVisible: false
    fullscreenMode: false
    panelColor: "transparent"
    panelBorderColor: "transparent"
    headerDragBarLength: 0
    siblingSnapAlignTop: true
    snapEdgeCenters: true

    function _fmt(v, unit) {
        return isNaN(v) ? "—" : (v.toFixed(1) + " " + unit)
    }

    function _applySize() {
        expandedWidth = _wantW
        expandedHeight = _wantH
    }

    property bool _synced: false

    function syncFromStore() {
        if (!popupVisible)
            return
        suspendSignals = true
        var p = store.autopilotPopupPosition(popupWidth, popupHeight)
        panelX = clampX(p.x)
        panelY = clampY(p.y)
        suspendSignals = false
        _synced = true
    }

    on_WantWChanged: _applySize()
    on_WantHChanged: _applySize()

    Component.onCompleted: {
        _applySize()
        syncFromStore()
        Qt.callLater(syncFromStore)
        Qt.callLater(resolveOverlapWithSibling)
    }

    onPopupVisibleChanged: {
        if (popupVisible) {
            _applySize()
            syncFromStore()
            Qt.callLater(syncFromStore)
            Qt.callLater(resolveOverlapWithSibling)
        }
    }

    onPositionCommitted: function(x, y, w, h) {
        if (_synced)
            store.setAutopilotPopupPosition(x, y, w, h)
    }

    dockState: store ? store.popupDock(popupId) : null
    onDockCommitted: function(targetId, side, gap, crossOffset) {
        store.setPopupDock(popupId, { targetId: targetId, side: side, gap: gap, cross: crossOffset })
    }

    component Cell: Row {
        property string caption: ""
        property string value: ""
        property color valueColor: AppPalette.text
        spacing: Math.round(3 * root._s)

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: parent.caption
            color: AppPalette.textMuted
            font.pixelSize: Math.round(9 * root._s)
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: parent.value
            color: parent.valueColor
            font.pixelSize: Math.round(11 * root._s)
            font.bold: true
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: root._panelRadius
        color: AppPalette.bg
        border.width: 1
        border.color: AppPalette.border

        Row {
            id: infoRow
            x: root._pad
            anchors.verticalCenter: parent.verticalCenter
            spacing: root._cellGap

            Cell { caption: qsTr("Battery"); value: root._fmt(root._voltage, "V") }
            Cell { caption: qsTr("Current"); value: root._fmt(root._current, "A") }
            Cell { caption: qsTr("Speed");   value: root._fmt(root._speed, "m/s") }
            Cell { caption: qsTr("Mode");    value: root._mode < 0 ? "—" : String(root._mode) }
            Cell {
                caption: qsTr("Arm")
                value: root._arm < 0 ? "—" : (root._arm > 0 ? "ARMED" : "DISARMED")
                valueColor: root._arm > 0 ? "#22C55E" : AppPalette.text
            }
        }

        KCircleIconButton {
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: root._sidePad
            width: root._controlH
            height: root._controlH
            iconSource: "qrc:/icons/ui/x.svg"
            iconTintColor: AppPalette.text
            fillColor: AppPalette.card
            fillHoverColor: AppPalette.cardHover
            fillPressedColor: AppPalette.bgDeep
            borderColor: AppPalette.border
            borderHoverColor: AppPalette.borderHover
            toolTipText: qsTr("Close")
            onClicked: if (root.store) root.store.autopilotEnabled = false
        }
    }
}
