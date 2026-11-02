.pragma library

function localPath(value) {
    if (!value)
        return ""
    if (typeof value === "string") {
        if (value.startsWith("file:///"))
            return Qt.platform.os === "windows" ? value.slice(8) : value.slice(7)
        if (value.startsWith("file://"))
            return value.slice(5)
        return value
    }
    var lp = value.toLocalFile ? value.toLocalFile() : ""
    return lp.length ? lp : value.toString()
}

function exportFileName(typed, fallbackStem, suffix, extension) {
    var name = String(typed === undefined || typed === null ? "" : typed).trim().replace(/\.(csv|xyz)$/i, "")
    if (!name.length)
        name = fallbackStem + suffix
    return name + "." + extension
}

function hasInvalidFileNameChars(name) {
    return /[<>:"|?*\\\/\x00-\x1F]/.test(name)
}
