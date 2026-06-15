.pragma library

function clamp(value, minValue, maxValue) {
    return Math.max(minValue, Math.min(maxValue, value))
}

function splitRectByHandle(node, x, y, w, h, outRects, outHandles, splitterThickness) {
    if (!node)
        return

    if (node.type === "leaf") {
        outRects.push({
            leafId: node.leafId,
            pane: node.pane,
            x: x,
            y: y,
            width: w,
            height: h
        })
        return
    }

    var gap = splitterThickness
    var ratio = clamp(node.ratio, 0.001, 0.999)

    if (node.orientation === "vertical") {
        var firstWidth = Math.max(0, w * ratio - gap / 2)
        var secondWidth = Math.max(0, w - firstWidth - gap)
        var splitX = x + firstWidth

        outHandles.push({
            splitId: node.splitId,
            orientation: "vertical",
            x: splitX,
            y: y,
            width: gap,
            height: h,
            parentLength: w
        })

        splitRectByHandle(node.first, x, y, firstWidth, h, outRects, outHandles, splitterThickness)
        splitRectByHandle(node.second, splitX + gap, y, secondWidth, h, outRects, outHandles, splitterThickness)
        return
    }

    var firstHeight = Math.max(0, h * ratio - gap / 2)
    var secondHeight = Math.max(0, h - firstHeight - gap)
    var splitY = y + firstHeight

    outHandles.push({
        splitId: node.splitId,
        orientation: "horizontal",
        x: x,
        y: splitY,
        width: w,
        height: gap,
        parentLength: h
    })

    splitRectByHandle(node.first, x, y, w, firstHeight, outRects, outHandles, splitterThickness)
    splitRectByHandle(node.second, x, splitY + gap, w, secondHeight, outRects, outHandles, splitterThickness)
}

function splitGeometryById(node, x, y, w, h, splitId, splitterThickness) {
    if (!node || node.type === "leaf")
        return null

    if (node.splitId === splitId) {
        if (node.orientation === "vertical") {
            return {
                orientation: "vertical",
                parentStart: x,
                parentLength: w
            }
        }

        return {
            orientation: "horizontal",
            parentStart: y,
            parentLength: h
        }
    }

    var gap = splitterThickness
    var ratio = clamp(node.ratio, 0.001, 0.999)

    if (node.orientation === "vertical") {
        var firstWidth = Math.max(0, w * ratio - gap / 2)
        var secondWidth = Math.max(0, w - firstWidth - gap)
        var splitX = x + firstWidth

        var leftGeo = splitGeometryById(node.first, x, y, firstWidth, h, splitId, splitterThickness)
        if (leftGeo)
            return leftGeo

        return splitGeometryById(node.second, splitX + gap, y, secondWidth, h, splitId, splitterThickness)
    }

    var firstHeight = Math.max(0, h * ratio - gap / 2)
    var secondHeight = Math.max(0, h - firstHeight - gap)
    var splitY = y + firstHeight

    var topGeo = splitGeometryById(node.first, x, y, w, firstHeight, splitId, splitterThickness)
    if (topGeo)
        return topGeo

    return splitGeometryById(node.second, x, splitY + gap, w, secondHeight, splitId, splitterThickness)
}

function subtreeMinSize(node, axis, minPaneSize, splitterThickness) {
    if (!node)
        return minPaneSize

    if (node.type === "leaf")
        return minPaneSize

    if (node.orientation === axis)
        return subtreeMinSize(node.first, axis, minPaneSize, splitterThickness)
                + splitterThickness
                + subtreeMinSize(node.second, axis, minPaneSize, splitterThickness)

    return Math.max(
                subtreeMinSize(node.first, axis, minPaneSize, splitterThickness),
                subtreeMinSize(node.second, axis, minPaneSize, splitterThickness)
                )
}
