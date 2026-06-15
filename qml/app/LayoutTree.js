.pragma library

function allLeafPanes(node, out) {
    if (!node)
        return
    if (node.type === "leaf") {
        out.push(node.pane)
        return
    }
    allLeafPanes(node.first, out)
    allLeafPanes(node.second, out)
}

function allLeafIds(node, out) {
    if (!node)
        return
    if (node.type === "leaf") {
        out.push(node.leafId)
        return
    }
    allLeafIds(node.first, out)
    allLeafIds(node.second, out)
}

function maxLeafIdInTree(node) {
    if (!node)
        return 0
    if (node.type === "leaf")
        return node.leafId > 0 ? node.leafId : 0
    return Math.max(maxLeafIdInTree(node.first), maxLeafIdInTree(node.second))
}

function maxSplitIdInTree(node) {
    if (!node || node.type === "leaf")
        return 0
    return Math.max(node.splitId > 0 ? node.splitId : 0, maxSplitIdInTree(node.first), maxSplitIdInTree(node.second))
}

function hasLeafIdInTree(node, leafId) {
    if (!node)
        return false
    if (node.type === "leaf")
        return node.leafId === leafId
    return hasLeafIdInTree(node.first, leafId) || hasLeafIdInTree(node.second, leafId)
}

function leafIdByPaneNumber(node, paneNumber) {
    if (!node)
        return -1
    if (node.type === "leaf")
        return node.pane && node.pane.paneId === paneNumber ? node.leafId : -1

    var left = leafIdByPaneNumber(node.first, paneNumber)
    if (left !== -1)
        return left
    return leafIdByPaneNumber(node.second, paneNumber)
}

function firstLeafIdByMode(node, mode) {
    if (!node)
        return -1
    if (node.type === "leaf")
        return ((node.pane && node.pane.mode === "3D") ? "3D" : "2D") === mode ? node.leafId : -1

    var left = firstLeafIdByMode(node.first, mode)
    if (left !== -1)
        return left
    return firstLeafIdByMode(node.second, mode)
}

function replaceLeaf(node, leafId, replacementNode) {
    if (!node)
        return node

    if (node.type === "leaf")
        return node.leafId === leafId ? replacementNode : node

    return {
        type: "split",
        splitId: node.splitId,
        orientation: node.orientation,
        ratio: node.ratio,
        first: replaceLeaf(node.first, leafId, replacementNode),
        second: replaceLeaf(node.second, leafId, replacementNode)
    }
}

function removeLeafFromNode(node, leafId) {
    if (!node)
        return null

    if (node.type === "leaf")
        return node.leafId === leafId ? null : node

    var first = removeLeafFromNode(node.first, leafId)
    var second = removeLeafFromNode(node.second, leafId)

    if (!first)
        return second
    if (!second)
        return first

    return {
        type: "split",
        splitId: node.splitId,
        orientation: node.orientation,
        ratio: node.ratio,
        first: first,
        second: second
    }
}

function updateSplitRatio(node, splitId, ratio) {
    if (!node)
        return node

    if (node.type === "leaf")
        return node

    var nextRatio = node.splitId === splitId ? ratio : node.ratio
    return {
        type: "split",
        splitId: node.splitId,
        orientation: node.orientation,
        ratio: nextRatio,
        first: updateSplitRatio(node.first, splitId, ratio),
        second: updateSplitRatio(node.second, splitId, ratio)
    }
}

function splitRatioById(node, splitId) {
    if (!node)
        return -1

    if (node.type === "leaf")
        return -1

    if (node.splitId === splitId)
        return node.ratio

    var left = splitRatioById(node.first, splitId)
    if (left >= 0)
        return left

    return splitRatioById(node.second, splitId)
}

function updatePaneInLeaf(node, leafId, paneObj) {
    if (!node)
        return node

    if (node.type === "leaf") {
        if (node.leafId === leafId) {
            return {
                type: "leaf",
                leafId: node.leafId,
                pane: paneObj
            }
        }
        return node
    }

    return {
        type: "split",
        splitId: node.splitId,
        orientation: node.orientation,
        ratio: node.ratio,
        first: updatePaneInLeaf(node.first, leafId, paneObj),
        second: updatePaneInLeaf(node.second, leafId, paneObj)
    }
}

function paneByLeafId(node, leafId) {
    if (!node)
        return null

    if (node.type === "leaf")
        return node.leafId === leafId ? node.pane : null

    var left = paneByLeafId(node.first, leafId)
    if (left)
        return left
    return paneByLeafId(node.second, leafId)
}

function findPathToLeaf(node, leafId, path) {
    if (!node)
        return false

    if (node.type === "leaf")
        return node.leafId === leafId

    path.push({ splitId: node.splitId, orientation: node.orientation, side: "first" })
    if (findPathToLeaf(node.first, leafId, path))
        return true
    path.pop()

    path.push({ splitId: node.splitId, orientation: node.orientation, side: "second" })
    if (findPathToLeaf(node.second, leafId, path))
        return true
    path.pop()

    return false
}

function splitNodeById(node, splitId) {
    if (!node || node.type === "leaf")
        return null

    if (node.splitId === splitId)
        return node

    var left = splitNodeById(node.first, splitId)
    if (left)
        return left

    return splitNodeById(node.second, splitId)
}

function firstAxisSplitFromBoundary(node, axis, boundarySide) {
    if (!node || node.type === "leaf")
        return null

    if (node.orientation === axis) {
        return {
            splitId: node.splitId,
            orientation: node.orientation,
            side: boundarySide === "start" ? "first" : "second"
        }
    }

    var next = boundarySide === "start" ? node.first : node.second
    return firstAxisSplitFromBoundary(next, axis, boundarySide)
}

function collectLeafIds(node, out) {
    if (!node)
        return
    if (node.type === "leaf") {
        out.push(node.leafId)
        return
    }
    collectLeafIds(node.first, out)
    collectLeafIds(node.second, out)
}
