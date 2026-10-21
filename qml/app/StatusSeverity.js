.pragma library

function color(palette, sev) {
    if (sev === "good")  return palette.linkOkBorder;
    if (sev === "warn")  return palette.linkIdleBorder;
    if (sev === "crit")  return palette.linkDownBorder;
    if (sev === "stale") return palette.textMuted;
    return palette.accentBar;
}

function text(palette, sev) {
    if (sev === "good")  return palette.linkOkText;
    if (sev === "warn")  return palette.linkIdleText;
    if (sev === "crit")  return palette.linkDownText;
    if (sev === "stale") return palette.textMuted;
    return palette.accentBar;
}

if (typeof module !== "undefined" && module.exports) {
    module.exports = { color: color, text: text };
}
