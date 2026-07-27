pragma Singleton
import QtQuick 2.15

QtObject {
    id: catalog

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

    function _meta(key) {
        for (var i = 0; i < fields.length; ++i)
            if (fields[i].key === key)
                return fields[i]
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

    function isValid(key, ds, dmw, store, sysbat) {
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

    function formatValue(key, ds, dmw, store, sysbat) {
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
