pragma Singleton
import QtQuick 2.15
import "UsblFieldLogic.js" as UsblLogic

QtObject {
    id: catalog

    // THE PALETTE MODEL. WidgetPlaceStep binds a Repeater straight to this, so adding an
    // entry here puts a tile in front of every user on every device — it is shared UI,
    // not a data declaration. Subsystem fields live in their own list below and are only
    // offered once something deliberately reads that list.
    readonly property var fields: [
        { key: "time",      label: qsTr("Time"),           unit: "",             group: "general" },
        { key: "depth",     label: qsTr("Depth"),          unit: qsTr("m"),      group: "general" },
        { key: "rfDepth",   label: qsTr("Rangefinder"),    unit: qsTr("m"),      group: "general" },
        { key: "btDepth",   label: qsTr("Bottom track"),   unit: qsTr("m"),      group: "general" },
        { key: "speed",     label: qsTr("Speed"),          unit: qsTr("km/h"),   group: "general" },
        { key: "apSpeed",   label: qsTr("Ground speed"),   unit: qsTr("m/s"),    group: "autopilot" },
        { key: "coord",     label: qsTr("Coordinate"),     unit: "",             group: "general" },
        { key: "selPoint",  label: qsTr("Selected point"), unit: "",             group: "general" },
        { key: "temp",      label: qsTr("Temperature"),    unit: "°C",           group: "general" },
        { key: "sysBattery",label: qsTr("Device charge"),  unit: "%",            group: "general" },
        { key: "apVoltage", label: qsTr("Battery"),        unit: "V",            group: "autopilot" },
        { key: "apCurrent", label: qsTr("Current"),        unit: "A",            group: "autopilot" },
        { key: "apMode",    label: qsTr("Flight mode"),    unit: "",             group: "autopilot" },
        { key: "apArm",     label: qsTr("Arm state"),      unit: "",             group: "autopilot" }
    ]

    // USBL fields — deliberately NOT in `fields`, so the palette is unchanged for the
    // vast majority of users who have no acoustic beacon.
    //
    // They are fully wired otherwise: hasField/label/unit/isValid/formatValue all resolve
    // them, so a cell referencing one renders correctly. What is missing is a way to
    // CREATE such a cell, and that waits on the palette expansion mechanism — one entry
    // per beacon that has actually answered, rather than ten dead tiles for everyone.
    //
    // `addressable: true` means the field answers "for WHICH beacon?", so a cell has to
    // carry an address alongside the key.
    readonly property var usblFields: [
        { key: "usblRange",       label: qsTr("Beacon range"),    unit: qsTr("m"),  group: "usbl", addressable: true },
        { key: "usblAzimuth",     label: qsTr("Beacon azimuth"),  unit: "°",        group: "usbl", addressable: true },
        { key: "usblElevation",   label: qsTr("Beacon elevation"),unit: "°",        group: "usbl", addressable: true },
        { key: "usblSnr",         label: qsTr("Acoustic SNR"),    unit: qsTr("dB"), group: "usbl", addressable: true },
        { key: "usblBeaconCoord", label: qsTr("Beacon position"), unit: "",         group: "usbl", addressable: true },
        { key: "usblBeaconDepth", label: qsTr("Beacon depth"),    unit: qsTr("m"),  group: "usbl", addressable: true },
        { key: "usblAge",         label: qsTr("Fix age"),         unit: qsTr("s"),  group: "usbl", addressable: true },
        { key: "usblState",       label: qsTr("Acoustic link"),   unit: "",         group: "usbl", addressable: true },
        { key: "usblAddress",     label: qsTr("Last beacon"),     unit: "",         group: "usbl" },
        { key: "usblPresent",     label: qsTr("USBL data"),       unit: "",         group: "usbl" }
    ]

    // Codes from UsblFieldLogic, translated here — the logic module never returns prose.
    readonly property var _usblStateText: ({
        "tracking": qsTr("TRACKING"),
        "stale":    qsTr("STALE"),
        "none":     qsTr("NO SOLUTION")
    })

    function isAddressable(key) { return UsblLogic.isAddressable(key) }
    function addressAny() { return UsblLogic.ANY }
    // A beacon's identity colour, constant for the life of that address. The badge in the
    // settings pane, the badge on the scene panel and (when it lands) the marker on the map
    // are the same beacon, so they take the same colour from one table.
    function usblAddressColor(addr) { return UsblLogic.addressColor(addr) }
    // Forwarded so WorkspaceStore has one owner of the rule rather than a second copy —
    // it cannot import this module's .js directly.
    function normAddr(a) { return UsblLogic.normAddr(a) }
    // The beacons that have actually answered — offering one that never has is offering a
    // permanently blank widget.
    function knownUsblAddresses(ds) {
        return ds ? UsblLogic.knownAddresses(ds.usblSolutions) : []
    }
    // Reading ds.usblSolutions HERE would not be tracked by the caller's binding; it is
    // read in the caller's expression via the `ds` parameter, same contract as the rest
    // of this file.
    function _usblEntry(ds, addr) {
        return ds ? UsblLogic.pick(ds.usblSolutions, addr) : null
    }
    // `store.nowMs` is WorkspaceStore's 1 s clock, and it has to exist: Date.now() inside a
    // binding is evaluated once and then frozen, so without a ticking property `usblAge` and
    // the tracking/stale flip in `usblState` would only advance when some OTHER dependency
    // changed. The fallback keeps a caller that passes no store rendering something sane.
    function _nowMs(store) {
        return (store && store.nowMs) ? store.nowMs : Date.now()
    }

    // Searches BOTH lists. `fields` is what the palette offers; this is what the app can
    // resolve. A cell referencing a USBL field renders correctly even though there is no
    // way to create one from the palette yet -- and normalizeWidgetDef keeps it on load
    // instead of dropping it as unknown.
    function _meta(key) {
        for (var i = 0; i < fields.length; ++i)
            if (fields[i].key === key)
                return fields[i]
        for (var j = 0; j < usblFields.length; ++j)
            if (usblFields[j].key === key)
                return usblFields[j]
        return null
    }

    function hasField(key) { return _meta(key) !== null }

    function label(key) {
        var m = _meta(key)
        return m ? m.label : key
    }

    function unit(key) {
        var m = _meta(key)
        return m ? m.unit : ""
    }

    function _dms(deg, isLat) {
        if (deg === undefined || deg === null || isNaN(deg))
            return ""
        var hemi = isLat ? (deg >= 0 ? "N" : "S") : (deg >= 0 ? "E" : "W")
        return hemi + " " + Math.abs(deg).toFixed(4) + "°"
    }

    function _autopilotValid(dmw) {
        if (!dmw)
            return false
        return (!isNaN(dmw.vruVoltage) || !isNaN(dmw.vruCurrent) || !isNaN(dmw.vruVelocityH)
                || dmw.pilotArmState >= 0 || dmw.pilotModeState >= 0)
    }

    // `addr` is the beacon a USBL cell is pinned to (-1 = whichever answered last). Every
    // other field ignores it, so existing call sites keep working unchanged.
    function isValid(key, ds, dmw, store, sysbat, addr) {
        if (UsblLogic.isUsblField(key)) {
            if (key === "usblPresent")
                return !!(ds && ds.hasUsblData)
            return UsblLogic.isValid(key, _usblEntry(ds, addr), _nowMs(store))
        }
        switch (key) {
        case "time":      return !!(store && store.systemTimeValid)
        case "sysBattery": return !!(sysbat && sysbat.available)
        case "depth":     return !!(ds && ds.isLastDepthValid)
        case "rfDepth":   return !!(ds && ds.isLastRangefinderDepthValid)
        case "btDepth":   return !!(ds && ds.isLastBottomTrackDepthValid)
        case "speed":     return !!(ds && ds.isSpeedValid && ds.isBoatCoordinateValid)
        case "coord":     return !!(ds && ds.isBoatCoordinateValid)
        case "selPoint":  return !!(ds && ds.isActiveContactIndxValid)
        case "temp":      return !!(ds && ds.isLastTempValid)
        case "apSpeed":   return _autopilotValid(dmw) && !isNaN(dmw.vruVelocityH)
        case "apVoltage": return _autopilotValid(dmw) && !isNaN(dmw.vruVoltage)
        case "apCurrent": return _autopilotValid(dmw) && !isNaN(dmw.vruCurrent)
        case "apMode":    return _autopilotValid(dmw) && dmw.pilotModeState >= 0
        case "apArm":     return _autopilotValid(dmw) && dmw.pilotArmState >= 0
        }
        return false
    }

    function formatValue(key, ds, dmw, store, sysbat, addr) {
        if (UsblLogic.isUsblField(key))
            return _formatUsbl(key, ds, store, addr)
        switch (key) {
        case "time":
            return (store && store.systemTimeValid) ? store.systemTimeHms : "—"
        case "sysBattery":
            return (sysbat && sysbat.available) ? (sysbat.level + " %") : "—"
        case "depth":
            return (ds && ds.isLastDepthValid) ? (ds.depth.toFixed(2) + " " + qsTr("m")) : "—"
        case "rfDepth":
            return (ds && ds.isLastRangefinderDepthValid) ? (ds.lastRangefinderDepth.toFixed(2) + " " + qsTr("m")) : "—"
        case "btDepth":
            return (ds && ds.isLastBottomTrackDepthValid) ? (ds.lastBottomTrackDepth.toFixed(2) + " " + qsTr("m")) : "—"
        case "speed":
            return (ds && ds.isSpeedValid && ds.isBoatCoordinateValid) ? (ds.speed.toFixed(1) + " " + qsTr("km/h")) : "—"
        case "coord":
            return (ds && ds.isBoatCoordinateValid) ? (_dms(ds.boatLatitude, true) + "\n" + _dms(ds.boatLongitude, false)) : "—"
        case "selPoint":
            return (ds && ds.isActiveContactIndxValid) ? (ds.distToContact.toFixed(1) + " " + qsTr("m") + "\n" + ds.angleToContact.toFixed(1) + "°") : "—"
        case "temp":
            return (ds && ds.isLastTempValid) ? (ds.lastTemp.toFixed(1) + " °C") : "—"
        case "apSpeed":
            return (dmw && !isNaN(dmw.vruVelocityH)) ? (dmw.vruVelocityH.toFixed(1) + " " + qsTr("m/s")) : "—"
        case "apVoltage":
            return (dmw && !isNaN(dmw.vruVoltage)) ? (dmw.vruVoltage.toFixed(1) + " V") : "—"
        case "apCurrent":
            return (dmw && !isNaN(dmw.vruCurrent)) ? (dmw.vruCurrent.toFixed(1) + " A") : "—"
        case "apMode":
            return (dmw && dmw.pilotModeState >= 0) ? String(dmw.pilotModeState) : "—"
        case "apArm":
            return (dmw && dmw.pilotArmState >= 0) ? (dmw.pilotArmState > 0 ? "ARMED" : "DISARMED") : "—"
        }
        return "—"
    }

    // Units and translated words are added HERE, never in the logic module: a unit baked
    // into the raw value could not be translated and would be wrong the day this grows a
    // feet/metres switch.
    function _formatUsbl(key, ds, store, addr) {
        if (key === "usblPresent")
            return (ds && ds.hasUsblData) ? qsTr("yes") : qsTr("no")

        var e = _usblEntry(ds, addr)
        var now = _nowMs(store)

        if (key === "usblState")
            return _usblStateText[UsblLogic.stateCode(e, now)] || "—"

        var v = UsblLogic.rawValue(key, e, now)
        if (v === null || v === undefined)
            return "—"

        switch (key) {
        case "usblRange":       return v + " " + qsTr("m")
        case "usblAzimuth":     return v + "°"
        case "usblElevation":   return v + "°"
        case "usblSnr":         return v + " " + qsTr("dB")
        case "usblBeaconDepth": return v + " " + qsTr("m")
        case "usblBeaconCoord": return v
        case "usblAge":         return (v < 10 ? v.toFixed(1) : Math.round(v)) + " " + qsTr("s")
        case "usblAddress":     return v
        }
        return String(v)
    }

    function sampleValue(key, store) {
        switch (key) {
        case "time":      return (store && store.systemTimeValid) ? store.systemTimeHms : "12:34:56"
        case "depth":     return "3.20 " + qsTr("m")
        case "rfDepth":   return "3.15 " + qsTr("m")
        case "btDepth":   return "3.24 " + qsTr("m")
        case "speed":     return "5.4 " + qsTr("km/h")
        case "apSpeed":   return "1.5 " + qsTr("m/s")
        case "coord":     return "N 40.6035°\nE 45.0010°"
        case "selPoint":  return "12.5 " + qsTr("m") + "\n135.0°"
        case "temp":      return "18.4 °C"
        case "sysBattery": return "87 %"
        case "apVoltage": return "12.4 V"
        case "apCurrent": return "3.2 A"
        case "apMode":    return "3"
        case "apArm":     return "ARMED"
        case "usblRange":       return "18.3 " + qsTr("m")
        case "usblAzimuth":     return "127.4°"
        case "usblElevation":   return "-22.8°"
        case "usblSnr":         return "24 " + qsTr("dB")
        case "usblBeaconCoord": return "N 59.9386°\nE 30.3141°"
        case "usblBeaconDepth": return "11.20 " + qsTr("m")
        case "usblAge":         return "1.4 " + qsTr("s")
        case "usblState":       return _usblStateText["tracking"]
        case "usblAddress":     return "2"
        case "usblPresent":     return qsTr("yes")
        }
        return "—"
    }

    function previewAvailable(key, sysbat) {
        if (key === "sysBattery")
            return !!(sysbat && sysbat.available)
        return true
    }

    function previewValue(key, store, sysbat) {
        if (key === "sysBattery")
            return formatValue(key, null, null, store, sysbat)
        return sampleValue(key, store)
    }
}
