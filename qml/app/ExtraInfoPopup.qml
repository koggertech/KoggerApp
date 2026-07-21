import QtQuick 2.15
import QtQuick.Layouts 1.15
import kqml_types 1.0

BasePanePopup {
    id: root

    required property var store

    readonly property real _s: AppPalette.appScale
    readonly property int _pad: Math.round(8 * _s)
    readonly property int _rowGap: Math.round(3 * _s)
    readonly property int _controlH: Math.round(36 * _s)
    readonly property int _sidePad: Math.round(3 * _s)
    readonly property int _panelRadius: Math.round((_controlH + _sidePad * 2) / 2)
    property real _panelAlpha: (store && store.extraInfoTransparencyEnabled && !revealActive)
                               ? Math.max(0.25, Math.min(1.0, store.extraInfoOpacity / 100))
                               : 1.0
    Behavior on _panelAlpha { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }

    readonly property var _ds: (typeof dataset !== "undefined") ? dataset : null
    readonly property var _dmw: (typeof deviceManagerWrapper !== "undefined") ? deviceManagerWrapper : null

    readonly property bool _depthValid: _ds ? _ds.isLastDepthValid : false
    readonly property bool _speedValid: _ds ? (_ds.isSpeedValid && _ds.isBoatCoordinateValid) : false
    readonly property bool _coordValid: _ds ? _ds.isBoatCoordinateValid : false
    readonly property bool _actValid:   _ds ? _ds.isActiveContactIndxValid : false
    readonly property bool _tempValid:  _ds ? _ds.isLastTempValid : false
    readonly property bool _rfValid:    _ds ? _ds.isLastRangefinderDepthValid : false
    readonly property bool _btValid:    _ds ? _ds.isLastBottomTrackDepthValid : false
    readonly property bool _timeValid:  !!(store && store.systemTimeValid)

    readonly property real _apVoltage: _dmw ? _dmw.vruVoltage : NaN
    readonly property real _apCurrent: _dmw ? _dmw.vruCurrent : NaN
    readonly property real _apSpeed:   _dmw ? _dmw.vruVelocityH : NaN
    readonly property int  _apArm:     _dmw ? _dmw.pilotArmState : -1
    readonly property int  _apMode:    _dmw ? _dmw.pilotModeState : -1
    readonly property bool _autopilotValid: !!(_dmw && (!isNaN(_apVoltage) || !isNaN(_apCurrent) || !isNaN(_apSpeed) || _apArm >= 0 || _apMode >= 0))

    function _fv(key, valid) {
        return !!(store && store.extraInfoFieldsMap !== undefined && store.extraInfoFieldEnabled(key))
    }
    readonly property bool _vTime:      _fv("time", _timeValid) && _timeValid
    readonly property bool _vDepth:     _fv("depth", _depthValid)
    readonly property bool _vSpeed:     _fv("speed", _speedValid)
    readonly property bool _vBoatLat:   _fv("boatLat", _coordValid)
    readonly property bool _vBoatLon:   _fv("boatLon", _coordValid)
    readonly property bool _vActDist:   _fv("actDist", _actValid)
    readonly property bool _vActAngle:  _fv("actAngle", _actValid)
    readonly property bool _vTemp:      _fv("temp", _tempValid)
    readonly property bool _vRfDepth:   _fv("rfDepth", _rfValid)
    readonly property bool _vBtDepth:   _fv("btDepth", _btValid)
    readonly property bool _vApVoltage: _fv("apVoltage", _autopilotValid)
    readonly property bool _vApCurrent: _fv("apCurrent", _autopilotValid)
    readonly property bool _vApSpeed:   _fv("apSpeed", _autopilotValid)
    readonly property bool _vApMode:    _fv("apMode", _autopilotValid)
    readonly property bool _vApArm:     _fv("apArm", _autopilotValid)
    // SimpleNavV2 per-field visibility hidden — uncomment to restore [nav-2/4]:
    /*
    readonly property bool _vNavFix:      _fv("navFix", _navValid)
    readonly property bool _vNavSats:     _fv("navSats", _navValid)
    readonly property bool _vNavTime:     _fv("navTime", _navValid)
    readonly property bool _vNavOffset:   _fv("navOffset", _navValid)
    readonly property bool _vNavLat:      _fv("navLat", _navValid)
    readonly property bool _vNavLon:      _fv("navLon", _navValid)
    readonly property bool _vNavCourse:   _fv("navCourse", _navValid)
    readonly property bool _vNavVelocity: _fv("navVelocity", _navValid)
    readonly property bool _vNavYaw:      _fv("navYaw", _navValid)
    readonly property bool _vNavPitch:    _fv("navPitch", _navValid)
    readonly property bool _vNavRoll:     _fv("navRoll", _navValid)
    */
    // Boat status per-field visibility hidden — uncomment to restore [bs-2/4]:
    /*
    readonly property bool _vBsBatBoat:   _fv("bsBatBoat", _bsValid)
    readonly property bool _vBsBatBridge: _fv("bsBatBridge", _bsValid)
    readonly property bool _vBsSigBoat:   _fv("bsSigBoat", _bsValid)
    readonly property bool _vBsSigBridge: _fv("bsSigBridge", _bsValid)
    */

    readonly property bool _coordAny: _vBoatLat || _vBoatLon
    readonly property bool _actAny:   _vActDist || _vActAngle
    readonly property bool _sensAny:  _vTemp || _vRfDepth || _vBtDepth
    readonly property bool _apAny:    _vApVoltage || _vApCurrent || _vApSpeed || _vApMode || _vApArm
    // SimpleNavV2 section aggregate hidden — uncomment to restore [nav-3/4]:
    // readonly property bool _navAny:   _vNavFix || _vNavSats || _vNavTime || _vNavOffset || _vNavLat || _vNavLon || _vNavCourse || _vNavVelocity || _vNavYaw || _vNavPitch || _vNavRoll
    // Boat status section aggregate hidden — uncomment to restore [bs-3/4]:
    // readonly property bool _bsAny:    _vBsBatBoat || _vBsBatBridge || _vBsSigBoat || _vBsSigBridge
    readonly property bool _anyVis: _vTime || _vDepth || _vSpeed || _coordAny || _actAny || _sensAny || _apAny /* || _navAny (SimpleNavV2 off) || _bsAny (Boat status off) */

    // GridLayout does not grow its implicit width for column-spanning section headers,
    // so a header wider than its (short) data rows would clip. Measure the widest visible
    // header (hidden probe below) and floor the grid width by it.
    readonly property real _hdrMaxW: Math.max(
        _coordAny ? _hpBoat.implicitWidth : 0,
        _actAny   ? _hpAct.implicitWidth  : 0,
        _sensAny  ? _hpSens.implicitWidth : 0,
        _apAny    ? _hpAp.implicitWidth   : 0
        // _navAny ? _hpNav.implicitWidth : 0,   // SimpleNavV2 off [nav]
        // _bsAny  ? _hpBs.implicitWidth  : 0     // Boat status off [bs]
    )
    readonly property real _gridW: Math.max(infoCol.implicitWidth, _hdrMaxW)

    popupVisible: store.extraInfoVisible && _anyVis
    dragHandleOpacity: _panelAlpha
    dragEnabled: true
    resizeEnabled: false
    collapseButtonVisible: false
    fullscreenMode: false
    panelColor: "transparent"
    panelBorderColor: "transparent"
    headerDragBarLength: 0
    siblingSnapAlignTop: true
    snapEdgeCenters: true

    readonly property real _cardW: Math.round(_gridW + _pad * 2)
    readonly property real _cardH: Math.round(Math.max(infoCol.implicitHeight + _pad * 2, _controlH + _sidePad * 2))
    readonly property real _wantW: _cardW + contentPadding * 2
    readonly property real _wantH: headerHeight + _cardH + contentPadding

    function _dms(deg, isLat) {
        if (deg === undefined || deg === null || isNaN(deg))
            return ""
        var hemi = isLat ? (deg >= 0 ? "N" : "S") : (deg >= 0 ? "E" : "W")
        return hemi + " " + Math.abs(deg).toFixed(4) + "°"
    }

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
        color: AppPalette.textSecond
        font.pixelSize: Math.round(13 * root._s)
        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
    }

    component Val: Text {
        color: AppPalette.textStrong
        font.pixelSize: Math.round(15 * root._s)
        font.bold: true
        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
        Layout.fillWidth: true
    }

    component Hdr: Text {
        color: AppPalette.textStrong
        font.pixelSize: Math.round(12 * root._s)
        font.bold: true
        Layout.columnSpan: 2
        Layout.fillWidth: true
        Layout.topMargin: Math.round(5 * root._s)
    }

    Rectangle {
        anchors.fill: parent
        radius: root._panelRadius
        color: Qt.rgba(AppPalette.bg.r, AppPalette.bg.g, AppPalette.bg.b, root._panelAlpha)
        border.width: 0

        // hidden probe: header text widths (drives root._hdrMaxW; see note above)
        Item {
            visible: false
            Text { id: _hpBoat; text: qsTr("Boat position"); font.pixelSize: Math.round(12 * root._s); font.bold: true }
            Text { id: _hpAct;  text: qsTr("Active point");  font.pixelSize: Math.round(12 * root._s); font.bold: true }
            Text { id: _hpSens; text: qsTr("Sensors");       font.pixelSize: Math.round(12 * root._s); font.bold: true }
            Text { id: _hpAp;   text: qsTr("Autopilot");     font.pixelSize: Math.round(12 * root._s); font.bold: true }
            // SimpleNavV2 header probe hidden — uncomment to restore [nav]:
            // Text { id: _hpNav;  text: qsTr("Navigation");    font.pixelSize: Math.round(12 * root._s); font.bold: true }
            // Boat status header probe hidden — uncomment to restore [bs]:
            // Text { id: _hpBs;   text: qsTr("Boat status");   font.pixelSize: Math.round(12 * root._s); font.bold: true }
        }

        GridLayout {
            id: infoCol
            x: root._pad
            width: root._gridW
            anchors.verticalCenter: parent.verticalCenter
            columns: 2
            rowSpacing: root._rowGap
            columnSpacing: Math.round(8 * root._s)

            Cap { visible: root._vTime; text: qsTr("Time") }
            Val { visible: root._vTime; text: root.store ? root.store.systemTimeHms : "" }
            Cap { visible: root._vDepth; text: qsTr("Depth") }
            Val { visible: root._vDepth; text: root._depthValid ? (root._ds.depth.toFixed(2) + " " + qsTr("m")) : "—" }
            Cap { visible: root._vSpeed; text: qsTr("Speed") }
            Val { visible: root._vSpeed; text: root._speedValid ? (root._ds.speed.toFixed(1) + " " + qsTr("km/h")) : "—" }

            Hdr { visible: root._coordAny; text: qsTr("Boat position") }
            Cap { visible: root._vBoatLat; text: qsTr("Lat") }
            Val { visible: root._vBoatLat; text: root._coordValid ? root._dms(root._ds.boatLatitude, true) : "—" }
            Cap { visible: root._vBoatLon; text: qsTr("Lon") }
            Val { visible: root._vBoatLon; text: root._coordValid ? root._dms(root._ds.boatLongitude, false) : "—" }

            Hdr { visible: root._actAny; text: qsTr("Active point") }
            Cap { visible: root._vActDist; text: qsTr("Distance") }
            Val { visible: root._vActDist; text: root._actValid ? (root._ds.distToContact.toFixed(1) + " " + qsTr("m")) : "—" }
            Cap { visible: root._vActAngle; text: qsTr("Angle") }
            Val { visible: root._vActAngle; text: root._actValid ? (root._ds.angleToContact.toFixed(1) + "°") : "—" }

            Hdr { visible: root._sensAny; text: qsTr("Sensors") }
            Cap { visible: root._vTemp; text: qsTr("Temperature") }
            Val { visible: root._vTemp; text: root._tempValid ? (root._ds.lastTemp.toFixed(1) + " °C") : "—" }
            Cap { visible: root._vRfDepth; text: qsTr("Rangefinder") }
            Val { visible: root._vRfDepth; text: root._rfValid ? (root._ds.lastRangefinderDepth.toFixed(2) + " " + qsTr("m")) : "—" }
            Cap { visible: root._vBtDepth; text: qsTr("Bottom track") }
            Val { visible: root._vBtDepth; text: root._btValid ? (root._ds.lastBottomTrackDepth.toFixed(2) + " " + qsTr("m")) : "—" }

            Hdr { visible: root._apAny; text: qsTr("Autopilot") }
            Cap { visible: root._vApVoltage; text: qsTr("Battery") }
            Val { visible: root._vApVoltage; text: root._fmt(root._apVoltage, "V") }
            Cap { visible: root._vApCurrent; text: qsTr("Current") }
            Val { visible: root._vApCurrent; text: root._fmt(root._apCurrent, "A") }
            Cap { visible: root._vApSpeed; text: qsTr("Speed") }
            Val { visible: root._vApSpeed; text: root._fmt(root._apSpeed, "m/s") }
            Cap { visible: root._vApMode; text: qsTr("Mode") }
            Val { visible: root._vApMode; text: root._apMode < 0 ? "—" : String(root._apMode) }
            Cap { visible: root._vApArm; text: qsTr("Arm") }
            Val { visible: root._vApArm; text: root._apArm < 0 ? "—" : (root._apArm > 0 ? "ARMED" : "DISARMED"); color: root._apArm > 0 ? "#22C55E" : AppPalette.textStrong }

            // ── SimpleNavV2 Navigation section: HIDDEN — uncomment block to restore [nav-4/4] ──
            /*
            Hdr { visible: root._navAny; text: qsTr("Navigation") }
            Cap { visible: root._vNavFix; text: qsTr("GNSS fix") }
            Val { visible: root._vNavFix; text: root._navValid ? String(root._ds.simpleNavV2GnssFixType) : "—" }
            Cap { visible: root._vNavSats; text: qsTr("Sats") }
            Val { visible: root._vNavSats; text: root._navValid ? String(root._ds.simpleNavV2NumSats) : "—" }
            Cap { visible: root._vNavTime; text: qsTr("Time") }
            Val { visible: root._vNavTime; text: root._navValid ? String(root._ds.simpleNavV2UnixTime) : "—" }
            Cap { visible: root._vNavOffset; text: qsTr("Offset") }
            Val { visible: root._vNavOffset; text: root._navValid ? (String(root._ds.simpleNavV2UnixOffsetMs) + " ms") : "—" }
            Cap { visible: root._vNavLat; text: qsTr("Lat") }
            Val { visible: root._vNavLat; text: root._navValid ? root._ds.simpleNavV2Latitude.toFixed(7) : "—" }
            Cap { visible: root._vNavLon; text: qsTr("Lon") }
            Val { visible: root._vNavLon; text: root._navValid ? root._ds.simpleNavV2Longitude.toFixed(7) : "—" }
            Cap { visible: root._vNavCourse; text: qsTr("Course") }
            Val { visible: root._vNavCourse; text: root._navValid ? (root._ds.simpleNavV2GroundCourseDeg.toFixed(2) + "°") : "—" }
            Cap { visible: root._vNavVelocity; text: qsTr("Velocity") }
            Val { visible: root._vNavVelocity; text: root._navValid ? (root._ds.simpleNavV2GroundVelocityMps.toFixed(3) + " m/s") : "—" }
            Cap { visible: root._vNavYaw; text: qsTr("Yaw") }
            Val { visible: root._vNavYaw; text: root._navValid ? (root._ds.simpleNavV2YawDeg.toFixed(2) + "°") : "—" }
            Cap { visible: root._vNavPitch; text: qsTr("Pitch") }
            Val { visible: root._vNavPitch; text: root._navValid ? (root._ds.simpleNavV2PitchDeg.toFixed(2) + "°") : "—" }
            Cap { visible: root._vNavRoll; text: qsTr("Roll") }
            Val { visible: root._vNavRoll; text: root._navValid ? (root._ds.simpleNavV2RollDeg.toFixed(2) + "°") : "—" }
            */

            // ── Boat status section: HIDDEN — uncomment block to restore [bs-4/4] ──
            /*
            Hdr { visible: root._bsAny; text: qsTr("Boat status") }
            Cap { visible: root._vBsBatBoat; text: qsTr("Battery (boat)") }
            Val { visible: root._vBsBatBoat; text: root._bsValid ? (root._ds.boatStatusBatteryBoatPercent + "%") : "—" }
            Cap { visible: root._vBsBatBridge; text: qsTr("Battery (bridge)") }
            Val { visible: root._vBsBatBridge; text: root._bsValid ? (root._ds.boatStatusBatteryBridgePercent + "%") : "—" }
            Cap { visible: root._vBsSigBoat; text: qsTr("Signal (boat)") }
            Val { visible: root._vBsSigBoat; text: root._bsValid ? (root._ds.boatStatusSignalQualityBoatPercent + "%") : "—" }
            Cap { visible: root._vBsSigBridge; text: qsTr("Signal (bridge)") }
            Val { visible: root._vBsSigBridge; text: root._bsValid ? (root._ds.boatStatusSignalQualityBridgePercent + "%") : "—" }
            */
        }
    }
}
