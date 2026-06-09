import QtQuick 2.15
import QtQuick.Layouts 1.15
import kqml_types 1.0

BasePanePopup {
    id: root

    required property var store

    readonly property real _s: 1.5 * (theme ? theme.resCoeff : 1.0)
    readonly property int _pad: Math.round(8 * _s)
    readonly property int _rowGap: Math.round(3 * _s)

    readonly property var _ds: (typeof dataset !== "undefined") ? dataset : null

    readonly property bool _depthValid: _ds ? _ds.isLastDepthValid : false
    readonly property bool _speedValid: _ds ? _ds.isSpeedValid : false
    readonly property bool _coordValid: _ds ? _ds.isBoatCoordinateValid : false
    readonly property bool _apValid:    _ds ? _ds.isActiveContactIndxValid : false
    readonly property bool _navValid:   _ds ? _ds.isSimpleNavV2Valid : false
    readonly property bool _bsValid:    _ds ? _ds.isBoatStatusValid : false

    readonly property bool _depthVis: store.extraInfoDepth && _depthValid
    readonly property bool _speedVis: store.extraInfoSpeed && _speedValid
    readonly property bool _coordVis: store.extraInfoCoordinates && _coordValid
    readonly property bool _apVis:    store.extraInfoActivePoint && _apValid
    readonly property bool _navVis:   store.extraInfoNav && _navValid
    readonly property bool _bsVis:    store.extraInfoBoatStatus && _bsValid
    readonly property bool _anyVis: _depthVis || _speedVis || _coordVis || _apVis || _navVis || _bsVis

    popupVisible: store.extraInfoVisible && _anyVis
    dragEnabled: true
    resizeEnabled: false
    collapseButtonVisible: false
    fullscreenMode: false
    panelColor: "transparent"
    panelBorderColor: "transparent"
    headerDragBarLength: 0
    siblingSnapAlignTop: true
    snapEdgeCenters: true

    readonly property real _cardW: Math.round(infoCol.implicitWidth + _pad * 2)
    readonly property real _cardH: Math.round(infoCol.implicitHeight + _pad * 2)
    readonly property real _wantW: _cardW + contentPadding * 2
    readonly property real _wantH: headerHeight + _cardH + contentPadding

    function _dms(deg, isLat) {
        if (deg === undefined || deg === null || isNaN(deg))
            return ""
        var hemi = isLat ? (deg >= 0 ? "N" : "S") : (deg >= 0 ? "E" : "W")
        return hemi + " " + Math.abs(deg).toFixed(4) + "°"
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
        var p = store.extraInfoPopupPosition(popupWidth, popupHeight)
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
            store.setExtraInfoPopupPosition(x, y, w, h)
    }

    dockState: store ? store.popupDock(popupId) : null
    onDockCommitted: function(targetId, side, gap, crossOffset) {
        store.setPopupDock(popupId, { targetId: targetId, side: side, gap: gap, cross: crossOffset })
    }

    component Cap: Text {
        color: AppPalette.textMuted
        font.pixelSize: Math.round(11 * root._s)
        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
    }

    component Val: Text {
        color: AppPalette.text
        font.pixelSize: Math.round(11 * root._s)
        font.bold: true
        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
    }

    component Hdr: Text {
        color: AppPalette.textSecond
        font.pixelSize: Math.round(10 * root._s)
        font.bold: true
        Layout.columnSpan: 2
        Layout.fillWidth: true
        Layout.topMargin: Math.round(5 * root._s)
    }

    Rectangle {
        anchors.fill: parent
        radius: Math.round(10 * root._s)
        color: AppPalette.bg
        border.width: 1
        border.color: AppPalette.border

        GridLayout {
            id: infoCol
            x: root._pad
            y: root._pad
            columns: 2
            rowSpacing: root._rowGap
            columnSpacing: Math.round(18 * root._s)

            Cap { visible: root._depthVis; text: qsTr("Depth") }
            Val { visible: root._depthVis; text: root._ds ? (root._ds.depth.toFixed(2) + " " + qsTr("m")) : "" }
            Cap { visible: root._speedVis; text: qsTr("Speed") }
            Val { visible: root._speedVis; text: root._ds ? (root._ds.speed.toFixed(1) + " " + qsTr("km/h")) : "" }

            Hdr { visible: root._coordVis; text: qsTr("Boat position") }
            Cap { visible: root._coordVis; text: qsTr("Lat") }
            Val { visible: root._coordVis; text: root._ds ? root._dms(root._ds.boatLatitude, true) : "" }
            Cap { visible: root._coordVis; text: qsTr("Lon") }
            Val { visible: root._coordVis; text: root._ds ? root._dms(root._ds.boatLongitude, false) : "" }

            Hdr { visible: root._apVis; text: qsTr("Active point") }
            Cap { visible: root._apVis; text: qsTr("Distance") }
            Val { visible: root._apVis; text: root._ds ? (root._ds.distToContact.toFixed(1) + " " + qsTr("m")) : "" }
            Cap { visible: root._apVis; text: qsTr("Angle") }
            Val { visible: root._apVis; text: root._ds ? (root._ds.angleToContact.toFixed(1) + "°") : "" }

            Hdr { visible: root._navVis; text: qsTr("Navigation") }
            Cap { visible: root._navVis; text: qsTr("GNSS fix") }
            Val { visible: root._navVis; text: root._ds ? String(root._ds.simpleNavV2GnssFixType) : "" }
            Cap { visible: root._navVis; text: qsTr("Sats") }
            Val { visible: root._navVis; text: root._ds ? String(root._ds.simpleNavV2NumSats) : "" }
            Cap { visible: root._navVis; text: qsTr("Time") }
            Val { visible: root._navVis; text: root._ds ? String(root._ds.simpleNavV2UnixTime) : "" }
            Cap { visible: root._navVis; text: qsTr("Offset") }
            Val { visible: root._navVis; text: root._ds ? (String(root._ds.simpleNavV2UnixOffsetMs) + " ms") : "" }
            Cap { visible: root._navVis; text: qsTr("Lat") }
            Val { visible: root._navVis; text: root._ds ? root._ds.simpleNavV2Latitude.toFixed(7) : "" }
            Cap { visible: root._navVis; text: qsTr("Lon") }
            Val { visible: root._navVis; text: root._ds ? root._ds.simpleNavV2Longitude.toFixed(7) : "" }
            Cap { visible: root._navVis; text: qsTr("Course") }
            Val { visible: root._navVis; text: root._ds ? (root._ds.simpleNavV2GroundCourseDeg.toFixed(2) + "°") : "" }
            Cap { visible: root._navVis; text: qsTr("Velocity") }
            Val { visible: root._navVis; text: root._ds ? (root._ds.simpleNavV2GroundVelocityMps.toFixed(3) + " m/s") : "" }
            Cap { visible: root._navVis; text: qsTr("Yaw") }
            Val { visible: root._navVis; text: root._ds ? (root._ds.simpleNavV2YawDeg.toFixed(2) + "°") : "" }
            Cap { visible: root._navVis; text: qsTr("Pitch") }
            Val { visible: root._navVis; text: root._ds ? (root._ds.simpleNavV2PitchDeg.toFixed(2) + "°") : "" }
            Cap { visible: root._navVis; text: qsTr("Roll") }
            Val { visible: root._navVis; text: root._ds ? (root._ds.simpleNavV2RollDeg.toFixed(2) + "°") : "" }

            Hdr { visible: root._bsVis; text: qsTr("Boat status") }
            Cap { visible: root._bsVis; text: qsTr("Battery (boat)") }
            Val { visible: root._bsVis; text: root._ds ? (root._ds.boatStatusBatteryBoatPercent + "%") : "" }
            Cap { visible: root._bsVis; text: qsTr("Battery (bridge)") }
            Val { visible: root._bsVis; text: root._ds ? (root._ds.boatStatusBatteryBridgePercent + "%") : "" }
            Cap { visible: root._bsVis; text: qsTr("Signal (boat)") }
            Val { visible: root._bsVis; text: root._ds ? (root._ds.boatStatusSignalQualityBoatPercent + "%") : "" }
            Cap { visible: root._bsVis; text: qsTr("Signal (bridge)") }
            Val { visible: root._bsVis; text: root._ds ? (root._ds.boatStatusSignalQualityBridgePercent + "%") : "" }
        }
    }
}
