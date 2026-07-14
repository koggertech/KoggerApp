.pragma library

function severity(dev) {
    if (!dev || !dev.recorderStatusValid)
        return "idle"
    var st = dev.recorderRecordingState
    if (st === 3 || dev.recorderCriticalFlags)
        return "crit"
    if (st === 4)
        return "crit"
    if (st === 2) {
        var active = dev.recorderStatusFlags || 0
        var silent = (dev.recorderDegradedFlags || 0) & 0x7
        if (dev.recorderSecondsSinceLastWrite > 10 || (silent && !active))
            return "warn"
        if (silent & ~active)
            return "warn"
        return "good"
    }
    return "idle"
}

function fmtSize(b) {
    if (!b || b <= 0) return "0 B"
    if (b < 1024) return b + " B"
    if (b < 1048576) return (b / 1024).toFixed(1) + " KB"
    if (b < 1073741824) return (b / 1048576).toFixed(1) + " MB"
    return (b / 1073741824).toFixed(2) + " GB"
}

function elapsed(s) {
    if (!s || s <= 0) return "0s"
    if (s < 60) return s + "s"
    if (s < 3600) return Math.floor(s / 60) + "m"
    return Math.floor(s / 3600) + "h"
}

function fmtSizeShort(b) {
    if (!b || b <= 0) return "0"
    if (b < 1048576)    return Math.round(b / 1024) + "K"
    if (b < 1073741824) return Math.round(b / 1048576) + "M"
    if (b < 1099511627776) {
        var g = b / 1073741824
        return (g < 9.95 ? g.toFixed(1) : Math.round(g)) + "G"
    }
    var t = b / 1099511627776
    return (t < 9.95 ? t.toFixed(1) : Math.round(t)) + "T"
}
