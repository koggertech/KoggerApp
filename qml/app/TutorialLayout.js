.pragma library

var SIDES = ["bottom", "top", "right", "left"]

function clamp(v, lo, hi) {
    if (hi < lo)
        return (lo + hi) / 2
    return Math.max(lo, Math.min(hi, v))
}

function opposite(side) {
    return side === "bottom" ? "top" : side === "top" ? "bottom" : side === "left" ? "right" : side === "right" ? "left" : ""
}

function sideForArrow(direction) {
    return direction === "down" ? "top" : direction === "up" ? "bottom" : direction === "left" ? "right" : direction === "right" ? "left" : ""
}

function arrowForSide(side) {
    return side === "bottom" ? "up" : side === "top" ? "down" : side === "right" ? "left" : side === "left" ? "right" : ""
}

function freeSpace(side, hole, area, margin) {
    if (side === "bottom")
        return area.height - (hole.y + hole.height) - margin
    if (side === "top")
        return hole.y - margin
    if (side === "right")
        return area.width - (hole.x + hole.width) - margin
    return hole.x - margin
}

function fits(side, hole, size, area, margin, gap) {
    var need = (side === "bottom" || side === "top") ? size.height : size.width
    return freeSpace(side, hole, area, margin) >= need + gap
}

function chooseSide(hole, size, area, margin, gap, preferred) {
    if (preferred === "center")
        return "center"
    if (preferred && SIDES.indexOf(preferred) >= 0 && fits(preferred, hole, size, area, margin, gap))
        return preferred
    var vertical = ["bottom", "top"].filter(function (s) { return fits(s, hole, size, area, margin, gap) })
    if (vertical.length > 0)
        return vertical.sort(function (a, b) { return freeSpace(b, hole, area, margin) - freeSpace(a, hole, area, margin) })[0]
    var horizontal = ["right", "left"].filter(function (s) { return fits(s, hole, size, area, margin, gap) })
    if (horizontal.length > 0)
        return horizontal.sort(function (a, b) { return freeSpace(b, hole, area, margin) - freeSpace(a, hole, area, margin) })[0]
    return "center"
}

function layoutBubble(opts) {
    var area = opts.area
    var size = opts.size
    var margin = opts.margin
    var hole = opts.hole
    var arrowOn = !!opts.arrowEnabled && !!hole
    var arrowGap = arrowOn ? opts.arrowLength + opts.arrowOffset + (opts.arrowTravel || 0) + opts.gap : opts.tailSize + opts.gap

    var centered = {
        x: clamp((area.width - size.width) / 2, margin, area.width - size.width - margin),
        y: clamp((area.height - size.height) / 2, margin, area.height - size.height - margin),
        side: "center", tailSide: "", tailOffset: 0,
        arrowVisible: false, tipX: 0, tipY: 0, direction: "down"
    }
    if (!hole || opts.placement === "center")
        return centered

    var preferred = opts.placement
    if ((preferred === "auto" || !preferred) && arrowOn && opts.arrowDirection !== "auto")
        preferred = sideForArrow(opts.arrowDirection)

    var side = chooseSide(hole, size, area, margin, arrowGap, preferred)
    if (side === "center")
        return centered

    var cx = hole.x + hole.width / 2
    var cy = hole.y + hole.height / 2
    var r = {
        x: 0, y: 0, side: side, tailSide: arrowOn ? "" : opposite(side), tailOffset: 0,
        arrowVisible: arrowOn, tipX: 0, tipY: 0, direction: arrowForSide(side)
    }

    if (side === "bottom" || side === "top") {
        r.x = clamp(cx - size.width / 2, margin, area.width - size.width - margin)
        r.y = side === "bottom" ? hole.y + hole.height + arrowGap : hole.y - arrowGap - size.height
        r.tailOffset = clamp(cx - r.x, opts.cornerRadius + opts.tailSize, size.width - opts.cornerRadius - opts.tailSize)
        r.tipX = clamp(cx, margin, area.width - margin)
        r.tipY = side === "bottom" ? hole.y + hole.height + opts.arrowOffset : hole.y - opts.arrowOffset
    } else {
        r.y = clamp(cy - size.height / 2, margin, area.height - size.height - margin)
        r.x = side === "right" ? hole.x + hole.width + arrowGap : hole.x - arrowGap - size.width
        r.tailOffset = clamp(cy - r.y, opts.cornerRadius + opts.tailSize, size.height - opts.cornerRadius - opts.tailSize)
        r.tipY = clamp(cy, margin, area.height - margin)
        r.tipX = side === "right" ? hole.x + hole.width + opts.arrowOffset : hole.x - opts.arrowOffset
    }
    return r
}

function holeFor(rect, shape, padding, radius) {
    var x = rect.x - padding
    var y = rect.y - padding
    var w = rect.width + padding * 2
    var h = rect.height + padding * 2
    if (shape === "circle") {
        var d = Math.max(w, h)
        return { x: x + (w - d) / 2, y: y + (h - d) / 2, width: d, height: d, radius: d / 2 }
    }
    return { x: x, y: y, width: w, height: h, radius: shape === "rect" ? 0 : radius }
}

function unite(rects) {
    if (rects.length === 0)
        return null
    var x1 = rects[0].x, y1 = rects[0].y
    var x2 = rects[0].x + rects[0].width, y2 = rects[0].y + rects[0].height
    for (var i = 1; i < rects.length; ++i) {
        x1 = Math.min(x1, rects[i].x)
        y1 = Math.min(y1, rects[i].y)
        x2 = Math.max(x2, rects[i].x + rects[i].width)
        y2 = Math.max(y2, rects[i].y + rects[i].height)
    }
    return { x: x1, y: y1, width: x2 - x1, height: y2 - y1 }
}

function uniteHoles(holes) {
    if (holes.length === 0)
        return []
    var u = unite(holes)
    var r = holes[0].radius
    for (var i = 1; i < holes.length; ++i)
        r = Math.min(r, holes[i].radius)
    u.radius = Math.min(r, u.width / 2, u.height / 2)
    return [u]
}

function mergeHoles(holes, gap) {
    var out = holes.slice()
    var merged = true
    while (merged) {
        merged = false
        for (var i = 0; i < out.length && !merged; ++i) {
            for (var j = i + 1; j < out.length && !merged; ++j) {
                var a = out[i], b = out[j]
                if (a.x < b.x + b.width + gap && b.x < a.x + a.width + gap
                        && a.y < b.y + b.height + gap && b.y < a.y + a.height + gap) {
                    out[i] = uniteHoles([a, b])[0]
                    out.splice(j, 1)
                    merged = true
                }
            }
        }
    }
    return out
}

function mergeOverlapping(holes) {
    return mergeHoles(holes, 0)
}

function intersect(a, b) {
    var x1 = Math.max(a.x, b.x)
    var y1 = Math.max(a.y, b.y)
    var x2 = Math.min(a.x + a.width, b.x + b.width)
    var y2 = Math.min(a.y + a.height, b.y + b.height)
    if (x2 - x1 < 1 || y2 - y1 < 1)
        return null
    return { x: x1, y: y1, width: x2 - x1, height: y2 - y1 }
}

function intersects(rect, area) {
    return rect.x < area.width && rect.y < area.height && rect.x + rect.width > 0 && rect.y + rect.height > 0
}

function sameRect(a, b) {
    if (!a || !b)
        return a === b
    return Math.abs(a.x - b.x) < 0.5 && Math.abs(a.y - b.y) < 0.5
        && Math.abs(a.width - b.width) < 0.5 && Math.abs(a.height - b.height) < 0.5
}
