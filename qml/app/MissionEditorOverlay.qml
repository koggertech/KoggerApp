import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import kqml_types 1.0

Rectangle {
    id: root

    required property var workspaceRoot

    readonly property bool active: typeof core !== "undefined" && core ? core.missionEditorActive : false
    readonly property var view: workspaceRoot ? workspaceRoot.scene3dViewItem : null
    readonly property var ctl: view ? view.missionController : null
    readonly property var plan: typeof missionPlan !== "undefined" ? missionPlan : null
    readonly property real _s: AppPalette.scale
    readonly property int toolSize: Tokens.controlHXl
    readonly property int headerBtn: toolSize
    readonly property bool tracing: ctl ? (ctl.tool === 6 || ctl.tool === 7 || ctl.tool === 9) : false
    readonly property int panelWidth: Math.min(Math.round(340 * _s), Math.round(width * 0.42))

    property int rev: 0
    property string draggingItemId: ""
    property var itemIds: []
    property var rallyIds: []
    property var fenceIds: []
    property var estimates: ({})
    property var groupedIssues: []
    property string _issuesKey: ""

    component ToolButton: KCircleIconButton {
        property bool armed: false
        width: root.toolSize
        height: root.toolSize
        iconTintColor: enabled ? AppPalette.text : AppPalette.textMuted
        glyphColor: enabled ? AppPalette.text : AppPalette.textMuted
        fillColor: armed ? AppPalette.accentBgStrong : AppPalette.card
        fillHoverColor: armed ? AppPalette.accentBorder : AppPalette.cardHover
        fillPressedColor: AppPalette.bgHover
        borderColor: armed ? AppPalette.accentBorder : AppPalette.border
        borderHoverColor: armed ? AppPalette.accentBorder : AppPalette.borderHover
        borderWidth: 1
    }

    visible: active
    enabled: active
    color: AppPalette.bg
    focus: active

    function refreshFromPlan() {
        if (!plan) return
        var ids = plan.itemIds()
        if (ids.join(",") !== itemIds.join(",")) itemIds = ids
        var rally = plan.rallyIds()
        if (rally.join(",") !== rallyIds.join(",")) rallyIds = rally
        var fences = plan.fenceIds()
        if (fences.join(",") !== fenceIds.join(",")) fenceIds = fences
        estimates = plan.estimates
        var issues = groupIssues(plan.issues)
        var key = JSON.stringify(issues)
        if (key !== _issuesKey) { _issuesKey = key; groupedIssues = issues }
        Qt.callLater(root._bumpRev)
        syncRouteModel()
    }

    function _bumpRev() { rev += 1 }

    function groupIssues(list) {
        var order = []
        var byId = {}
        for (var i = 0; i < list.length; ++i) {
            var it = list[i]
            var key = it.itemId || ""
            if (!(key in byId)) {
                byId[key] = { itemId: key, level: 0, lines: [] }
                order.push(key)
            }
            byId[key].lines.push(it.text)
            byId[key].level = Math.max(byId[key].level, it.level)
        }
        var out = []
        for (var k = 0; k < order.length; ++k) {
            var g = byId[order[k]]
            var text = g.lines.length > 1 ? g.lines.map(function(t) { return "• " + t }).join("\n") : g.lines[0]
            out.push({ itemId: g.itemId, level: g.level, text: text })
        }
        out.sort(function(a, b) {
            var ai = a.itemId.length ? itemIds.indexOf(a.itemId) : itemIds.length
            var bi = b.itemId.length ? itemIds.indexOf(b.itemId) : itemIds.length
            return ai - bi
        })
        return out
    }

    function syncRouteModel() {
        if (draggingItemId.length) return
        for (var i = routeModel.count - 1; i >= 0; --i) {
            if (itemIds.indexOf(routeModel.get(i).itemId) < 0) routeModel.remove(i)
        }
        for (var j = 0; j < itemIds.length; ++j) {
            var at = -1
            for (var k = 0; k < routeModel.count; ++k) {
                if (routeModel.get(k).itemId === itemIds[j]) { at = k; break }
            }
            if (at < 0) routeModel.insert(j, { itemId: itemIds[j] })
            else if (at !== j) routeModel.move(at, j, 1)
        }
    }

    function commitRouteOrder(id) {
        for (var i = 0; i < routeModel.count; ++i) {
            if (routeModel.get(i).itemId === id) {
                if (plan && plan.indexOfItem(id) !== i) plan.moveItem(id, i)
                return
            }
        }
    }

    function updateDragOrder(card) {
        if (!card || !card.dragActive) return
        var count = routeModel.count
        if (count < 2) return
        var probeY = card.y + card.height / 2 + routeListView.contentY
        var target = routeListView.indexAt(1, probeY)
        if (target < 0) {
            if (probeY < 0) target = 0
            else if (probeY > routeListView.contentHeight) target = count - 1
            else return
        }
        target = Math.max(0, Math.min(count - 1, target))
        var from = card.visualIndex
        if (target === from) return
        routeModel.move(from, target, 1)
        if (plan) plan.moveItem(card.itemId, target)
    }

    function moveRouteItem(id, delta) {
        for (var i = 0; i < routeModel.count; ++i) {
            if (routeModel.get(i).itemId !== id) continue
            var to = i + delta
            if (to < 0 || to >= routeModel.count) return
            routeModel.move(i, to, 1)
            if (plan) plan.moveItem(id, to)
            return
        }
    }

    function itemData(id) {
        void rev
        if (!plan || !id) return null
        var s = plan.itemJson(id)
        if (!s || !s.length) return null
        try { return JSON.parse(s) } catch (e) { return null }
    }

    function itemInfo(id) {
        void rev
        return plan ? plan.itemInfo(id) : ({})
    }

    function patch(id, obj) {
        if (plan) plan.updateItem(id, obj)
    }

    function formatLength(m) {
        if (!isFinite(m)) return "—"
        return m >= 1000 ? (m / 1000).toFixed(2) + " " + qsTr("km") : Math.round(m) + " " + qsTr("m")
    }

    function formatTime(sec) {
        if (!isFinite(sec)) return "—"
        var s = Math.round(sec)
        var h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60), r = s % 60
        if (h > 0) return qsTr("%1 h %2 min").arg(h).arg(m)
        if (m > 0) return qsTr("%1 min %2 s").arg(m).arg(r)
        return qsTr("%1 s").arg(r)
    }

    function itemTitle(id, index) {
        var d = itemData(id)
        var t = d ? typeLabel(String(d.type)) : qsTr("Item")
        return t + " " + (index + 1)
    }

    function typeLabel(t) {
        return t === "waypoint" ? qsTr("Waypoint")
             : t === "survey"   ? qsTr("Survey")
             : t === "corridor" ? qsTr("Corridor")
             : t === "rally"    ? qsTr("Rally point")
             : t === "fence"    ? qsTr("Geofence")
             : t
    }

    function fileBaseName(path) {
        var p = String(path)
        try { p = decodeURIComponent(p) } catch (e) {}
        p = p.replace(/\\/g, "/")
        var cut = Math.max(p.lastIndexOf("/"), p.lastIndexOf(":"))
        return p.substring(cut + 1)
    }

    function saveNew(after) {
        if (!plan) return
        if (Qt.platform.os === "android") {
            var target = plan.suggestedFilePath()
            if (plan.saveFileAs(target)) {
                if (typeof notifications !== "undefined" && notifications) notifications.info(qsTr("Saved to %1").arg(target))
                if (after) after()
            }
            return
        }
        pendingAfterSave = after
        _prepareDialog(saveDialog)
        saveDialog.open()
    }

    function directoryUrl() {
        return plan ? plan.directoryUrl() : ""
    }

    function openShapeChooser(anchorItem, tool) {
        shapeChooser.tool = tool
        var p = anchorItem.mapToItem(root, anchorItem.width, 0)
        shapeChooser.x = p.x + Tokens.spaceSm
        shapeChooser.y = Math.max(Tokens.spaceMd, Math.min(root.height - shapeChooser.height - Tokens.spaceMd, p.y))
        shapeChooser.visible = true
    }

    property var pendingAfterSave: null

    function guardUnsaved(action) {
        if (plan && plan.dirty) {
            closePrompt.afterAction = action
            closePrompt.visible = true
            return
        }
        action()
    }

    function requestClose() {
        if (plan && plan.dirty) {
            closePrompt.visible = true
            return
        }
        closeNow()
    }

    function closeNow() {
        closePrompt.visible = false
        if (typeof core !== "undefined" && core) core.setMissionEditorActive(false)
    }

    function handleEscape(sweep) {
        if (!active) return false
        if (closePrompt.visible) { closePrompt.visible = false; return true }
        if (ctl && ctl.tool !== 0) { ctl.tool = 0; return true }
        if (ctl && ctl.selectedId && ctl.selectedId.length) { ctl.clearSelection(); return true }
        if (sweep) return false
        requestClose()
        return true
    }

    function _takeHost() {
        if (!workspaceRoot) return
        workspaceRoot.active3DHostItem = hostSurface
    }

    function _releaseHost() {
        if (!workspaceRoot) return
        if (workspaceRoot.active3DHostItem === hostSurface)
            workspaceRoot.active3DHostItem = null
        var stacks = workspaceRoot.paneHostStacksByLeafId
        for (var key in stacks) {
            if (Object.prototype.hasOwnProperty.call(stacks, key))
                workspaceRoot.refreshHostAssignmentsForLeaf(parseInt(key))
        }
    }

    onActiveChanged: {
        if (active) {
            refreshFromPlan()
            _takeHost()
            hostBridge.forceActiveFocus()
        } else {
            _releaseHost()
        }
    }

    Connections {
        target: root.workspaceRoot
        ignoreUnknownSignals: true
        function onActive3DHostItemChanged() {
            if (root.active && root.workspaceRoot.active3DHostItem !== hostSurface)
                root.workspaceRoot.active3DHostItem = hostSurface
        }
    }

    Connections {
        target: root.plan
        ignoreUnknownSignals: true
        function onPlanChanged() { if (root.active) root.refreshFromPlan() }
        function onLastErrorChanged() {
            if (root.plan && root.plan.lastError && root.plan.lastError.length && typeof notifications !== "undefined" && notifications)
                notifications.warning(root.plan.lastError)
        }
    }


    Timer {
        id: revealTimer
        interval: 30
        onTriggered: root.revealSelected()
    }

    function revealSelected() {
        if (!ctl || !ctl.selectedId || !ctl.selectedId.length) return
        var id = ctl.selectedId
        var item = null
        var idx = itemIds.indexOf(id)
        if (idx >= 0) item = routeListView.itemAtIndex(idx)
        else {
            var r = rallyIds.indexOf(id)
            if (r >= 0) item = rallyRepeater.itemAt(r)
            var f = fenceIds.indexOf(id)
            if (f >= 0) item = fenceRepeater.itemAt(f)
        }
        if (!item) return
        var top = item.mapToItem(panelFlick.contentItem, 0, 0).y
        var bottom = top + item.height
        var margin = Tokens.spaceLg
        var maxY = Math.max(0, panelFlick.contentHeight - panelFlick.height)
        var y = panelFlick.contentY
        if (top - margin < y || item.height + 2 * margin > panelFlick.height) y = top - margin
        else if (bottom + margin > y + panelFlick.height) y = bottom + margin - panelFlick.height
        var target = Math.max(0, Math.min(maxY, y))
        if (Math.abs(target - panelFlick.contentY) < 1) return
        revealAnim.stop()
        revealAnim.from = panelFlick.contentY
        revealAnim.to = target
        revealAnim.start()
    }

    NumberAnimation {
        id: revealAnim
        target: panelFlick
        property: "contentY"
        duration: 220
        easing.type: Easing.OutCubic
    }

    Shortcut { sequences: [StandardKey.Undo]; enabled: root.active; context: Qt.ApplicationShortcut; onActivated: if (root.plan) root.plan.undo() }
    Shortcut { sequences: [StandardKey.Redo]; enabled: root.active; context: Qt.ApplicationShortcut; onActivated: if (root.plan) root.plan.redo() }
    Shortcut { sequences: [StandardKey.Save]; enabled: root.active; context: Qt.ApplicationShortcut; onActivated: root.saveAction() }

    function saveAction() {
        if (!plan) return
        if (plan.filePath && plan.filePath.length) plan.saveFile()
        else saveNew(null)
    }

    MouseArea {
        anchors.fill: parent
        z: -1
        acceptedButtons: Qt.AllButtons
        hoverEnabled: true
        preventStealing: true
        propagateComposedEvents: false
        onPressed:       function(mouse) { mouse.accepted = true }
        onReleased:      function(mouse) { mouse.accepted = true }
        onClicked:       function(mouse) { mouse.accepted = true }
        onDoubleClicked: function(mouse) { mouse.accepted = true }
        onPressAndHold:  function(mouse) { mouse.accepted = true }
        onWheel:         function(wheel) { wheel.accepted = true }
    }

    ListModel { id: routeModel }

    FileDialog {
        id: openDialog
        title: qsTr("Open mission")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Mission files (*.kmission)"), qsTr("All files (*)")]
        onAccepted: {
            if (!root.plan) return
            root.plan.rememberDirectoryOf(selectedFile)
            if (root.plan.openFile(selectedFile) && root.ctl) root.ctl.fitToPlan()
        }
    }

    FileDialog {
        id: saveDialog
        title: qsTr("Save mission")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Mission files (*.kmission)")]
        defaultSuffix: "kmission"
        onAccepted: {
            var after = root.pendingAfterSave
            root.pendingAfterSave = null
            if (!root.plan) return
            root.plan.rememberDirectoryOf(selectedFile)
            if (root.plan.saveFileAs(selectedFile)) {
                closePrompt.afterAction = null
                if (after) after()
            } else if (after) {
                closePrompt.visible = true
            }
        }
        onRejected: { root.pendingAfterSave = null; if (closePrompt.afterAction) closePrompt.visible = true }
    }

    FileDialog {
        id: exportPlanDialog
        title: qsTr("Export QGroundControl plan")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("QGC plan (*.plan)")]
        defaultSuffix: "plan"
        onAccepted: if (root.plan) root.plan.exportPlanFile(selectedFile)
    }

    FileDialog {
        id: exportWplDialog
        title: qsTr("Export Mission Planner waypoints")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Waypoints (*.waypoints)"), qsTr("Text (*.txt)")]
        defaultSuffix: "waypoints"
        onAccepted: if (root.plan) root.plan.exportWplFile(selectedFile)
    }

    function _prepareDialog(dlg) {
        var u = root.directoryUrl()
        if (u.length) dlg.currentFolder = u
        if (dlg.fileMode === FileDialog.SaveFile && root.plan)
            dlg.selectedFile = (u.length ? u + "/" : "") + root.plan.defaultFileName().replace(/\.kmission$/, "")
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: header
            Layout.fillWidth: true
            Layout.preferredHeight: root.toolSize + Tokens.spaceMd * 2
            color: AppPalette.headerBg

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Tokens.spaceLg
                anchors.rightMargin: Tokens.spaceLg
                spacing: Tokens.spaceMd

                Text {
                    text: qsTr("Mission")
                    color: AppPalette.textStrong
                    font.pixelSize: Tokens.fontXl
                    font.bold: true
                }

                Rectangle {
                    Layout.preferredWidth: Math.round(220 * root._s)
                    Layout.preferredHeight: Tokens.controlHMd
                    radius: Tokens.radiusMd
                    color: AppPalette.card
                    border.color: nameInput.activeFocus ? AppPalette.borderFocus : AppPalette.border
                    border.width: 1

                    TextInput {
                        id: nameInput
                        anchors.fill: parent
                        anchors.leftMargin: Tokens.spaceMd
                        anchors.rightMargin: Tokens.spaceMd
                        verticalAlignment: TextInput.AlignVCenter
                        color: AppPalette.textStrong
                        font.pixelSize: Tokens.fontBase
                        selectByMouse: true
                        clip: true
                        text: root.plan ? root.plan.name : ""
                        onEditingFinished: if (root.plan && root.plan.name !== text) root.plan.name = text
                    }
                }

                Text {
                    text: {
                        if (!root.plan) return ""
                        var where = root.plan.filePath.length ? root.fileBaseName(root.plan.filePath) : qsTr("not saved yet")
                        return root.plan.dirty ? where + " • " + qsTr("unsaved changes") : where
                    }
                    color: root.plan && root.plan.dirty ? AppPalette.linkIdleText : AppPalette.textMuted
                    font.pixelSize: Tokens.fontSm
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }

                KCircleIconButton {
                    Layout.preferredWidth: root.headerBtn; Layout.preferredHeight: root.headerBtn; Layout.minimumWidth: root.headerBtn; Layout.minimumHeight: root.headerBtn; iconPixelSize: Tokens.iconLg
                    iconSource: "qrc:/icons/ui/file_plus.svg"; iconTintColor: AppPalette.text
                    fillColor: AppPalette.card; fillHoverColor: AppPalette.cardHover; borderColor: AppPalette.border; borderWidth: 1
                    toolTipText: qsTr("New mission")
                    onClicked: root.guardUnsaved(function() { if (root.plan) root.plan.newPlan() })
                }
                KCircleIconButton {
                    Layout.preferredWidth: root.headerBtn; Layout.preferredHeight: root.headerBtn; Layout.minimumWidth: root.headerBtn; Layout.minimumHeight: root.headerBtn; iconPixelSize: Tokens.iconLg
                    iconSource: "qrc:/icons/ui/folder-open.svg"; iconTintColor: AppPalette.text
                    fillColor: AppPalette.card; fillHoverColor: AppPalette.cardHover; borderColor: AppPalette.border; borderWidth: 1
                    toolTipText: qsTr("Open mission")
                    onClicked: root.guardUnsaved(function() { root._prepareDialog(openDialog); openDialog.open() })
                }
                KCircleIconButton {
                    Layout.preferredWidth: root.headerBtn; Layout.preferredHeight: root.headerBtn; Layout.minimumWidth: root.headerBtn; Layout.minimumHeight: root.headerBtn; iconPixelSize: Tokens.iconLg
                    iconSource: "qrc:/icons/ui/file-check.svg"; iconTintColor: AppPalette.text
                    borderWidth: 1
                    fillColor: root.plan && root.plan.dirty ? AppPalette.accentBgStrong : AppPalette.card
                    fillHoverColor: root.plan && root.plan.dirty ? AppPalette.accentBorder : AppPalette.cardHover
                    borderColor: root.plan && root.plan.dirty ? AppPalette.accentBorder : AppPalette.border
                    toolTipText: qsTr("Save mission")
                    onClicked: root.saveAction()
                }
                KCircleIconButton {
                    id: moreBtn
                    Layout.preferredWidth: root.headerBtn; Layout.preferredHeight: root.headerBtn; Layout.minimumWidth: root.headerBtn; Layout.minimumHeight: root.headerBtn; iconPixelSize: Tokens.iconLg
                    iconSource: "qrc:/icons/ui/menu-2.svg"; iconTintColor: AppPalette.text
                    fillColor: moreMenu.visible ? AppPalette.accentBgStrong : AppPalette.card
                    fillHoverColor: AppPalette.cardHover; borderColor: AppPalette.border; borderWidth: 1
                    toolTipText: qsTr("Save as, export")
                    onClicked: {
                        if (!moreMenu.visible) {
                            var p = moreBtn.mapToItem(root, 0, 0)
                            moreMenu.x = Math.max(Tokens.spaceMd, Math.min(root.width - moreMenu.width - Tokens.spaceMd, p.x + moreBtn.width - moreMenu.width))
                        }
                        moreMenu.visible = !moreMenu.visible
                    }
                }

                KCircleIconButton {
                    Layout.preferredWidth: root.headerBtn; Layout.minimumWidth: root.headerBtn
                    Layout.preferredHeight: root.headerBtn; Layout.minimumHeight: root.headerBtn
                    glyph: "✕"
                    glyphPixelSize: Tokens.fontXl
                    fillColor: AppPalette.card; fillHoverColor: AppPalette.cardHover; borderColor: AppPalette.border; borderWidth: 1
                    toolTipText: qsTr("Close editor")
                    onClicked: root.requestClose()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                id: toolsColumn
                Layout.preferredWidth: root.toolSize + Tokens.spaceMd * 2
                Layout.fillHeight: true
                color: AppPalette.bgDeep

                Flickable {
                    id: toolsFlick
                    anchors.fill: parent
                    contentWidth: width
                    contentHeight: toolsList.implicitHeight
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    interactive: contentHeight > height

                    readonly property bool overflow: contentHeight > height + 1
                    readonly property bool atTop: contentY <= 1
                    readonly property bool atBottom: contentY >= contentHeight - height - 1

                    Column {
                        id: toolsList
                        anchors.horizontalCenter: parent.horizontalCenter
                        topPadding: Tokens.spaceLg
                        bottomPadding: Tokens.spaceLg
                        spacing: Tokens.spaceSm

                        Repeater {
                            model: [
                                { tool: 1, icon: "qrc:/icons/ui/home.svg",              tip: qsTr("Start point") },
                                { tool: 2, icon: "qrc:/icons/ui/map-pin-plus.svg",      tip: qsTr("Waypoint") },
                                { tool: 3, icon: "qrc:/icons/ui/lasso-polygon.svg",     tip: qsTr("Survey area") },
                                { tool: 4, icon: "qrc:/icons/ui/vector-bezier-arc.svg", tip: qsTr("Corridor") },
                                { tool: 5, icon: "qrc:/icons/ui/pennant.svg",           tip: qsTr("Rally point") },
                                { tool: 8, icon: "qrc:/icons/ui/shield.svg",            tip: qsTr("Geofence") }
                            ]
                            delegate: ToolButton {
                                id: toolBtn
                                required property var modelData
                                readonly property bool attention: modelData.tool === 1 && root.plan && !root.plan.hasHome && !armed
                                readonly property bool shapeTool: modelData.tool === 3 || modelData.tool === 4 || modelData.tool === 8
                                readonly property int traceTool: modelData.tool === 3 ? 6 : modelData.tool === 4 ? 7 : 9
                                armed: root.ctl && (root.ctl.tool === modelData.tool || (shapeTool && root.ctl.tool === traceTool))
                                iconSource: modelData.icon
                                toolTipText: modelData.tip
                                borderColor: armed ? AppPalette.accentBorder : (attention ? AppPalette.linkIdleBorder : AppPalette.border)
                                borderWidth: attention ? 2 : 1
                                onClicked: {
                                    if (!root.ctl) return
                                    if (shapeTool) {
                                        if (root.ctl.tool === traceTool) { root.ctl.cancelDraft(); return }
                                        root.openShapeChooser(toolBtn, modelData.tool)
                                        return
                                    }
                                    root.ctl.tool = armed ? 0 : modelData.tool
                                }
                            }
                        }

                        Item { width: 1; height: Tokens.spaceSm; visible: root.tracing }

                        ToolButton {
                            visible: root.tracing
                            iconSource: "qrc:/icons/ui/check.svg"
                            iconPixelSize: Math.round(root.toolSize * 0.5)
                            toolTipText: qsTr("Finish shape")
                            enabled: root.ctl && root.ctl.draftReady
                            fillColor: enabled ? AppPalette.linkOkBg : AppPalette.card
                            fillHoverColor: enabled ? Qt.lighter(AppPalette.linkOkBg, 1.25) : AppPalette.cardHover
                            fillPressedColor: enabled ? Qt.darker(AppPalette.linkOkBg, 1.15) : AppPalette.bgHover
                            borderColor: enabled ? AppPalette.linkOkBorder : AppPalette.border
                            borderHoverColor: enabled ? Qt.lighter(AppPalette.linkOkBorder, 1.2) : AppPalette.borderHover
                            onClicked: if (root.ctl) root.ctl.finishDraft()
                        }
                        ToolButton {
                            visible: root.tracing
                            glyph: "✕"; glyphPixelSize: Tokens.fontXxl
                            toolTipText: qsTr("Cancel shape")
                            fillColor: AppPalette.linkDownBg
                            fillHoverColor: Qt.lighter(AppPalette.linkDownBg, 1.25)
                            fillPressedColor: Qt.darker(AppPalette.linkDownBg, 1.15)
                            borderColor: AppPalette.linkDownBorder
                            borderHoverColor: Qt.lighter(AppPalette.linkDownBorder, 1.2)
                            onClicked: if (root.ctl) root.ctl.cancelDraft()
                        }
                        ToolButton {
                            visible: root.tracing
                            glyph: "⌫"; glyphPixelSize: Tokens.fontXl
                            toolTipText: qsTr("Remove last vertex")
                            enabled: root.ctl && root.ctl.draftCount > 0
                            onClicked: if (root.ctl) root.ctl.undoDraftVertex()
                        }

                        Item { width: 1; height: Tokens.spaceLg }

                        ToolButton {
                            iconSource: "qrc:/icons/ui/arrow-back-up.svg"
                            toolTipText: qsTr("Undo")
                            enabled: root.plan && root.plan.canUndo
                            onClicked: if (root.plan) root.plan.undo()
                        }
                        ToolButton {
                            iconSource: "qrc:/icons/ui/arrow-forward-up.svg"
                            toolTipText: qsTr("Redo")
                            enabled: root.plan && root.plan.canRedo
                            onClicked: if (root.plan) root.plan.redo()
                        }
                        Item { width: 1; height: Tokens.spaceLg }

                        ToolButton {
                            iconSource: "qrc:/icons/ui/fit-in-view.svg"
                            toolTipText: qsTr("Fit mission in view")
                            onClicked: if (root.ctl) root.ctl.fitToPlan()
                        }
                        ToolButton {
                            iconSource: "qrc:/icons/ui/plus.svg"
                            toolTipText: qsTr("Zoom in")
                            onClicked: if (root.view && root.view.zoomButtonAnimated) root.view.zoomButtonAnimated(4)
                        }
                        ToolButton {
                            iconSource: "qrc:/icons/ui/minus.svg"
                            toolTipText: qsTr("Zoom out")
                            onClicked: if (root.view && root.view.zoomButtonAnimated) root.view.zoomButtonAnimated(-4)
                        }

                        Item { width: 1; height: Tokens.spaceLg }

                        ToolButton {
                            id: layerBtn
                            iconSource: "qrc:/icons/ui/layers-selected.svg"
                            toolTipText: (typeof core !== "undefined" && core) ? core.mapTileProviderName : ""
                            toolTipSuppressed: layerSwitch.listShown || layerSwitch.listPinned
                            onPressStarted: holdTimer.restart()
                            onPressEnded: { holdTimer.stop(); Qt.callLater(function() { layerSwitch.swallowClick = false }) }
                            onPressCanceled: { holdTimer.stop(); Qt.callLater(function() { layerSwitch.swallowClick = false }) }
                            onClicked: {
                                if (layerSwitch.swallowClick) { layerSwitch.swallowClick = false; return }
                                if (layerSwitch.listPinned) { layerSwitch.listPinned = false; layerSwitch.listShown = false; return }
                                layerSwitch.quickSwitch()
                            }
                        }
                    }
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    height: Math.round(36 * root._s)
                    visible: toolsFlick.overflow && !toolsFlick.atTop
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: AppPalette.bgDeep }
                        GradientStop { position: 1.0; color: Qt.rgba(AppPalette.bgDeep.r, AppPalette.bgDeep.g, AppPalette.bgDeep.b, 0) }
                    }
                }
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: Math.round(36 * root._s)
                    visible: toolsFlick.overflow && !toolsFlick.atBottom
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: Qt.rgba(AppPalette.bgDeep.r, AppPalette.bgDeep.g, AppPalette.bgDeep.b, 0) }
                        GradientStop { position: 1.0; color: AppPalette.bgDeep }
                    }
                }
            }

            Item {
                id: sceneArea
                Layout.fillWidth: true
                Layout.fillHeight: true

                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: Math.round(28 * root._s)
                    z: 1
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#66000000" }
                        GradientStop { position: 1.0; color: "#00000000" }
                    }
                }
                Rectangle {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: Math.round(28 * root._s)
                    z: 1
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#00000000" }
                        GradientStop { position: 1.0; color: "#66000000" }
                    }
                }
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    height: Math.round(28 * root._s)
                    z: 1
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#66000000" }
                        GradientStop { position: 1.0; color: "#00000000" }
                    }
                }

                Item {
                    id: hostSurface
                    anchors.fill: parent
                    clip: true
                }

                PaneInputBridge {
                    id: hostBridge
                    anchors.fill: parent
                    workspaceRoot: root.workspaceRoot
                    leafId: -1
                    paneKind: "3D"
                    doubleTapEnabled: false
                    focusOnPointer: false
                    focusOnPress: true
                    active: root.active
                }

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: Tokens.spaceLg
                    width: Math.min(parent.width - Tokens.spaceXl * 2, hintText.implicitWidth + Tokens.spaceXl * 2)
                    height: hintText.implicitHeight + Tokens.spaceMd * 2
                    radius: Tokens.radiusLg
                    color: AppPalette.card
                    border.color: AppPalette.border
                    border.width: 1
                    visible: hintText.text.length > 0

                    Text {
                        id: hintText
                        anchors.centerIn: parent
                        width: parent.width - Tokens.spaceXl * 2
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        color: AppPalette.textStrong
                        font.pixelSize: Tokens.fontLg
                        text: !root.ctl ? ""
                            : root.ctl.tool === 1 ? qsTr("Tap the map to place the start point")
                            : root.ctl.tool === 2 ? qsTr("Tap the map to add waypoints")
                            : root.ctl.tool === 5 ? qsTr("Tap the map to add rally points")
                            : root.ctl.tool === 6 ? (root.ctl.draftReady ? qsTr("Tap to add corners, ✓ finishes the area") : qsTr("Tap the map to outline the area (3 corners or more)"))
                            : root.ctl.tool === 7 ? (root.ctl.draftReady ? qsTr("Tap to extend the axis, ✓ finishes the corridor") : qsTr("Tap the map along the corridor axis (2 points or more)"))
                            : root.ctl.tool === 9 ? (root.ctl.draftReady ? qsTr("Tap to add corners, ✓ finishes the zone") : qsTr("Tap the map to outline the zone (3 corners or more)"))
                            : root.ctl.selectedId.length ? qsTr("Drag handles to edit, + inserts a vertex")
                            : qsTr("Pick a tool on the left or tap an item to select it")
                    }
                }

                Item {
                    id: layerSwitch
                    anchors.fill: parent
                    z: 2

                    readonly property var providers: (typeof core !== "undefined" && core) ? core.mapTileProviders : []
                    readonly property int currentId: (typeof core !== "undefined" && core) ? core.mapTileProviderId : -1
                    property bool listShown: false
                    property bool listPinned: false
                    property bool swallowClick: false
                    readonly property bool hoverAny: layerBtn.hovered || listHover.hovered
                    readonly property bool listOpen: listShown || listPinned

                    onHoverAnyChanged: {
                        if (hoverAny) { closeTimer.stop(); listShown = true }
                        else closeTimer.restart()
                    }

                    onListOpenChanged: if (listOpen) placeList()

                    function placeList() {
                        var p = layerBtn.mapToItem(layerSwitch, layerBtn.width, layerBtn.height)
                        layerList.x = Math.max(0, p.x + Tokens.spaceSm)
                        layerList.y = Math.max(Tokens.spaceMd, Math.min(layerSwitch.height - layerList.height - Tokens.spaceMd, p.y - layerList.height))
                    }

                    function switchTo(id) {
                        if (id < 0 || typeof core === "undefined" || !core) return
                        listPinned = false
                        core.setMapTileProvider(id)
                    }
                    function quickSwitch() {
                        if (typeof core === "undefined" || !core) return
                        listPinned = false
                        core.switchToPreviousMapTileProvider()
                    }

                    Timer {
                        id: closeTimer
                        interval: 300
                        onTriggered: if (!layerSwitch.listPinned) layerSwitch.listShown = false
                    }
                    Timer {
                        id: holdTimer
                        interval: 450
                        onTriggered: {
                            layerSwitch.swallowClick = true
                            layerSwitch.listPinned = true
                            layerSwitch.listShown = true
                        }
                    }

                    Rectangle {
                        id: layerList
                        width: Math.round(200 * root._s)
                        height: layerColumn.implicitHeight + Tokens.spaceSm * 2
                        radius: Tokens.radiusMd
                        color: AppPalette.card
                        border.color: AppPalette.border
                        border.width: 1
                        visible: layerSwitch.listOpen
                        onHeightChanged: if (visible) layerSwitch.placeList()

                        HoverHandler { id: listHover }

                        Column {
                            id: layerColumn
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: Tokens.spaceSm

                            Repeater {
                                model: layerSwitch.providers
                                Rectangle {
                                    id: layerRow
                                    required property var modelData
                                    readonly property bool current: modelData.id === layerSwitch.currentId
                                    width: parent.width
                                    height: Tokens.controlHMd + Tokens.spaceXs
                                    radius: Tokens.radiusSm
                                    color: current ? AppPalette.accentBg : (rowArea.containsMouse ? AppPalette.cardHover : "transparent")

                                    Column {
                                        anchors.left: parent.left
                                        anchors.leftMargin: Tokens.spaceMd
                                        anchors.right: parent.right
                                        anchors.rightMargin: Tokens.spaceMd
                                        anchors.verticalCenter: parent.verticalCenter
                                        Text {
                                            width: parent.width
                                            elide: Text.ElideRight
                                            text: layerRow.modelData.name
                                            color: layerRow.current ? AppPalette.accentText : AppPalette.textStrong
                                            font.pixelSize: Tokens.fontBase
                                        }
                                        Text {
                                            text: layerRow.modelData.layer_type
                                            color: layerRow.current ? AppPalette.accentText : AppPalette.textMuted
                                            font.pixelSize: Tokens.fontSm
                                        }
                                    }

                                    MouseArea {
                                        id: rowArea
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: layerSwitch.switchTo(layerRow.modelData.id)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                id: panel
                Layout.preferredWidth: root.panelWidth
                Layout.fillHeight: true
                color: AppPalette.bgDeep

                Flickable {
                    id: panelFlick
                    anchors.fill: parent
                    anchors.margins: Tokens.spaceMd
                    contentWidth: width
                    contentHeight: panelColumn.implicitHeight
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    readonly property bool overflow: contentHeight > height + 1
                    readonly property bool atTop: contentY <= 1
                    readonly property bool atBottom: contentY >= contentHeight - height - 1

                    Column {
                        id: panelColumn
                        width: panelFlick.width
                        spacing: Tokens.spaceMd

                        KIsland {
                            title: qsTr("Plan")
                            width: parent.width

                            KIslandRow {
                                label: qsTr("Cruise speed, m/s")
                                KSpinBox {
                                    id: cruiseSpin
                                    width: Math.round(110 * root._s)
                                    height: Tokens.controlHMd
                                    from: 1; to: 300; stepSize: 1; divisor: 10; decimals: 1
                                    onValueModified: function(v) { if (root.plan) root.plan.cruiseSpeed = v / 10 }
                                }
                                Binding {
                                    target: cruiseSpin
                                    property: "value"
                                    value: root.plan ? Math.round(root.plan.cruiseSpeed * 10) : 15
                                }
                            }
                            KIslandRow {
                                label: qsTr("At the end")
                                KCombo {
                                    id: endActionCombo
                                    readonly property var actions: [2, 0, 1]
                                    width: Math.round(190 * root._s)
                                    model: [qsTr("Return to start point"), qsTr("Return to launch (RTL)"), qsTr("Hold position")]
                                    onActivated: function(i) { if (root.plan) root.plan.endAction = actions[i] }
                                }
                                Binding {
                                    target: endActionCombo
                                    property: "currentIndex"
                                    value: root.plan ? Math.max(0, endActionCombo.actions.indexOf(root.plan.endAction)) : 0
                                }
                            }
                            KIslandRow {
                                visible: root.plan && root.plan.endAction === 0
                                caption: qsTr("RTL goes to the autopilot's arming position, which may differ from the start point")
                                captionColor: AppPalette.linkIdleText
                                stacked: true
                            }
                            KIslandRow {
                                id: homeRow
                                readonly property bool missing: root.plan ? !root.plan.hasHome : false
                                property real pulse: 0.0
                                label: qsTr("Start point")
                                caption: missing ? qsTr("not set — tap here or use the tool on the left") : qsTr("placed")
                                labelColor: missing ? AppPalette.linkIdleText : AppPalette.textStrong
                                captionColor: missing ? AppPalette.linkIdleText : AppPalette.textSecond
                                fillColor: missing ? Qt.rgba(AppPalette.linkIdleBorder.r, AppPalette.linkIdleBorder.g, AppPalette.linkIdleBorder.b, pulse) : "transparent"
                                interactive: true
                                onClicked: {
                                    if (!root.ctl) return
                                    if (missing) root.ctl.tool = 1
                                    else root.ctl.select("home", -1)
                                }

                                SequentialAnimation on pulse {
                                    running: homeRow.missing && root.active
                                    loops: Animation.Infinite
                                    NumberAnimation { from: 0.04; to: 0.28; duration: 1100; easing.type: Easing.InOutSine }
                                    NumberAnimation { from: 0.28; to: 0.04; duration: 1100; easing.type: Easing.InOutSine }
                                }
                                onMissingChanged: if (!missing) pulse = 0.0

                                KCircleIconButton {
                                    visible: !homeRow.missing
                                    width: Tokens.controlHMd
                                    height: Tokens.controlHMd
                                    iconPixelSize: Tokens.iconSm
                                    iconTintColor: AppPalette.text
                                    fillColor: AppPalette.card
                                    fillHoverColor: AppPalette.cardHover
                                    borderColor: AppPalette.border
                                    borderWidth: 1
                                    iconSource: "qrc:/icons/ui/current-location.svg"
                                    toolTipText: qsTr("Show on map")
                                    onClicked: if (root.ctl) root.ctl.showItem("home")
                                }
                            }
                            Item {
                                visible: root.plan && root.plan.hasHome
                                width: parent.width
                                height: homeCoords.implicitHeight + Tokens.spaceSm * 2
                                MissionCoordFields {
                                    id: homeCoords
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.margins: Tokens.spaceMd
                                    anchors.verticalCenter: parent.verticalCenter
                                    lat: root.plan ? root.plan.homeLat : NaN
                                    lon: root.plan ? root.plan.homeLon : NaN
                                    onCommitted: function(la, lo) { if (root.plan) root.plan.setHome(la, lo) }
                                }
                            }
                        }

                        KIsland {
                            id: routeIsland
                            title: qsTr("Route")
                            width: parent.width
                            footer: root.itemIds.length === 0 ? qsTr("No items yet. Use the tools on the left.") : ""

                            Item {
                                id: routeDragLayer
                                width: parent ? parent.width : 0
                                height: routeListView.contentHeight

                                ListView {
                                    id: routeListView
                                    anchors.fill: parent
                                    interactive: false
                                    spacing: Tokens.spaceSm
                                    model: routeModel
                                    cacheBuffer: 100000

                                    move: Transition {
                                        NumberAnimation { properties: "y"; duration: 160; easing.type: Easing.OutCubic }
                                    }
                                    moveDisplaced: Transition {
                                        NumberAnimation { properties: "y"; duration: 160; easing.type: Easing.OutCubic }
                                    }

                                    delegate: Item {
                                        id: routeSlot
                                        required property string itemId
                                        required property int index
                                        width: routeListView.width
                                        height: routeCard.height
                                        property int visualIndex: index

                                        MissionItemCard {
                                            id: routeCard
                                            width: routeSlot.width
                                            editor: root
                                            itemId: routeSlot.itemId
                                            ordinal: routeSlot.visualIndex + 1
                                            isRally: false
                                            dragLayer: routeDragLayer
                                            visualIndex: routeSlot.visualIndex
                                            onExpanded: revealTimer.restart()
                                            onHandlePressChanged: function(pressed) { panelFlick.interactive = !pressed }
                                            onDragStarted: { root.draggingItemId = routeSlot.itemId; if (root.plan) root.plan.beginTransaction() }
                                            onDragMoved: root.updateDragOrder(routeCard)
                                            onDragFinished: {
                                                var id = routeSlot.itemId
                                                root.draggingItemId = ""
                                                root.commitRouteOrder(id)
                                                if (root.plan) root.plan.endTransaction(true)
                                                root.syncRouteModel()
                                                Qt.callLater(function() { routeCard.width = Qt.binding(function() { return routeSlot.width }) })
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        KIsland {
                            title: qsTr("Rally points")
                            width: parent.width
                            visible: root.rallyIds.length > 0

                            Repeater {
                                id: rallyRepeater
                                model: root.rallyIds
                                delegate: MissionItemCard {
                                    required property string modelData
                                    required property int index
                                    width: parent ? parent.width : 0
                                    editor: root
                                    itemId: modelData
                                    ordinal: index + 1
                                    isRally: true
                                    onExpanded: revealTimer.restart()
                                }
                            }
                        }

                        KIsland {
                            title: qsTr("Geofence")
                            width: parent.width
                            visible: root.fenceIds.length > 0

                            Repeater {
                                id: fenceRepeater
                                model: root.fenceIds
                                delegate: MissionItemCard {
                                    required property string modelData
                                    required property int index
                                    width: parent ? parent.width : 0
                                    editor: root
                                    itemId: modelData
                                    ordinal: index + 1
                                    isRally: false
                                    onExpanded: revealTimer.restart()
                                }
                            }
                        }

                        KIsland {
                            title: qsTr("Summary")
                            width: parent.width

                            KIslandRow { label: qsTr("Length");    caption: root.formatLength(root.estimates.lengthMeters) }
                            KIslandRow { label: qsTr("Time");      caption: root.formatTime(root.estimates.timeSeconds) }
                            KIslandRow { label: qsTr("Waypoints"); caption: String(root.estimates.waypointCount !== undefined ? root.estimates.waypointCount : 0) }
                            KIslandRow {
                                readonly property int count: root.estimates.flatItemCount !== undefined ? root.estimates.flatItemCount + (root.plan && root.plan.hasHome ? 1 : 0) : 0
                                readonly property int cap: root.plan ? root.plan.maxMissionItems : 0
                                label: qsTr("Mission items")
                                caption: count + " / " + cap
                                captionColor: count > cap ? AppPalette.linkDownText : (count > cap * 0.7 ? AppPalette.linkIdleText : AppPalette.textSecond)
                            }
                            Repeater {
                                model: root.groupedIssues
                                delegate: KIslandRow {
                                    required property var modelData
                                    readonly property string itemId: modelData.itemId
                                    readonly property bool isError: modelData.level === 2
                                    readonly property int itemIndex: itemId.length ? root.itemIds.indexOf(itemId) : -1
                                    label: itemIndex >= 0 ? root.itemTitle(itemId, itemIndex) : qsTr("Mission")
                                    caption: modelData.text
                                    labelColor: isError ? AppPalette.linkDownText : AppPalette.linkIdleText
                                    captionColor: isError ? AppPalette.linkDownText : AppPalette.linkIdleText
                                    stacked: true
                                    interactive: itemIndex >= 0
                                    chevron: itemIndex >= 0
                                    onClicked: if (itemIndex >= 0 && root.ctl) root.ctl.select(itemId, -1)
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    height: Math.round(36 * root._s)
                    visible: panelFlick.overflow && !panelFlick.atTop
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: AppPalette.bgDeep }
                        GradientStop { position: 1.0; color: Qt.rgba(AppPalette.bgDeep.r, AppPalette.bgDeep.g, AppPalette.bgDeep.b, 0) }
                    }
                }
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: Math.round(36 * root._s)
                    visible: panelFlick.overflow && !panelFlick.atBottom
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: Qt.rgba(AppPalette.bgDeep.r, AppPalette.bgDeep.g, AppPalette.bgDeep.b, 0) }
                        GradientStop { position: 1.0; color: AppPalette.bgDeep }
                    }
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        visible: moreMenu.visible || shapeChooser.visible
        onClicked: { moreMenu.visible = false; shapeChooser.visible = false }
    }

    Rectangle {
        id: shapeChooser
        property int tool: 3
        visible: false
        width: Math.round(280 * root._s)
        height: chooserColumn.implicitHeight + Tokens.spaceSm * 2
        radius: Tokens.radiusLg
        color: AppPalette.card
        border.color: AppPalette.border
        border.width: 1

        Column {
            id: chooserColumn
            anchors.fill: parent
            anchors.margins: Tokens.spaceSm

            Text {
                width: parent.width
                leftPadding: Tokens.spaceMd
                topPadding: Tokens.spaceXs
                bottomPadding: Tokens.spaceXs
                text: shapeChooser.tool === 3 ? qsTr("Survey area") : shapeChooser.tool === 4 ? qsTr("Corridor") : qsTr("Geofence")
                color: AppPalette.textMuted
                font.pixelSize: Tokens.fontSm
            }

            Repeater {
                model: shapeChooser.tool === 3
                       ? [ { label: qsTr("Rectangle here, then drag the corners"), mode: "template" },
                           { label: qsTr("Circle here, then drag the handles"),     mode: "circle" },
                           { label: qsTr("Outline it point by point"),              mode: "trace" } ]
                       : shapeChooser.tool === 4
                       ? [ { label: qsTr("Axis here, then drag the ends"),          mode: "template" },
                           { label: qsTr("Draw the axis point by point"),           mode: "trace" } ]
                       : [ { label: qsTr("Inclusion zone: rectangle here"),         mode: "template", inclusion: true },
                           { label: qsTr("Inclusion zone: outline point by point"), mode: "trace",    inclusion: true },
                           { label: qsTr("Exclusion zone: rectangle here"),         mode: "template", inclusion: false },
                           { label: qsTr("Exclusion zone: circle here"),            mode: "circle",   inclusion: false },
                           { label: qsTr("Exclusion zone: outline point by point"), mode: "trace",    inclusion: false } ]
                delegate: Rectangle {
                    required property var modelData
                    width: chooserColumn.width
                    height: Tokens.rowH
                    radius: Tokens.radiusMd
                    color: rowTap.containsMouse ? AppPalette.cardHover : "transparent"

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: Tokens.spaceMd
                        anchors.right: parent.right
                        anchors.rightMargin: Tokens.spaceMd
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.label
                        elide: Text.ElideRight
                        color: AppPalette.textStrong
                        font.pixelSize: Tokens.fontBase
                    }

                    MouseArea {
                        id: rowTap
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            shapeChooser.visible = false
                            if (!root.ctl) return
                            if (shapeChooser.tool === 8) {
                                if (modelData.mode === "template") root.ctl.placeFenceTemplate(modelData.inclusion)
                                else if (modelData.mode === "circle") root.ctl.placeFenceCircle(modelData.inclusion)
                                else { root.ctl.draftFenceInclusion = modelData.inclusion; root.ctl.tool = 9 }
                                return
                            }
                            if (modelData.mode === "template") {
                                if (shapeChooser.tool === 3) root.ctl.placeSurveyTemplate()
                                else root.ctl.placeCorridorTemplate()
                            }
                            else if (modelData.mode === "circle") root.ctl.placeSurveyCircle()
                            else root.ctl.tool = shapeChooser.tool === 3 ? 6 : 7
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id: moreMenu
        visible: false
        y: header.height + Tokens.spaceXs
        width: Math.round(300 * root._s)
        height: moreColumn.implicitHeight + Tokens.spaceSm * 2
        radius: Tokens.radiusLg
        color: AppPalette.card
        border.color: AppPalette.border
        border.width: 1

        Column {
            id: moreColumn
            anchors.fill: parent
            anchors.margins: Tokens.spaceSm

            Repeater {
                model: [
                    { label: qsTr("Save as…"),                          exportOnly: false, action: function() { root._prepareDialog(saveDialog); saveDialog.open() } },
                    { label: qsTr("Export for QGroundControl (.plan)"), exportOnly: true,  action: function() { root._prepareDialog(exportPlanDialog); exportPlanDialog.open() } },
                    { label: qsTr("Export for Mission Planner (.waypoints)"), exportOnly: true, action: function() { root._prepareDialog(exportWplDialog); exportWplDialog.open() } }
                ]
                delegate: Rectangle {
                    id: menuRow
                    required property var modelData
                    readonly property bool blocked: modelData.exportOnly && !!root.plan && !root.plan.exportable
                    width: moreColumn.width
                    height: blocked ? Tokens.rowH + Tokens.spaceMd : Tokens.rowH
                    radius: Tokens.radiusMd
                    color: menuRowTap.containsMouse && !blocked ? AppPalette.cardHover : "transparent"
                    opacity: blocked ? 0.55 : 1.0

                    Column {
                        anchors.left: parent.left
                        anchors.leftMargin: Tokens.spaceMd
                        anchors.right: parent.right
                        anchors.rightMargin: Tokens.spaceMd
                        anchors.verticalCenter: parent.verticalCenter
                        Text {
                            width: parent.width
                            elide: Text.ElideRight
                            text: menuRow.modelData.label
                            color: AppPalette.textStrong
                            font.pixelSize: Tokens.fontBase
                        }
                        Text {
                            width: parent.width
                            visible: menuRow.blocked
                            elide: Text.ElideRight
                            text: root.plan ? root.plan.exportBlocker : ""
                            color: AppPalette.linkDownText
                            font.pixelSize: Tokens.fontSm
                        }
                    }

                    MouseArea {
                        id: menuRowTap
                        anchors.fill: parent
                        hoverEnabled: true
                        enabled: !menuRow.blocked
                        onClicked: { moreMenu.visible = false; menuRow.modelData.action() }
                    }
                }
            }
        }
    }

    Rectangle {
        id: closePrompt
        anchors.fill: parent
        color: AppPalette.dim
        visible: false
        property var afterAction: null

        MouseArea { anchors.fill: parent; onClicked: closePrompt.visible = false }

        Rectangle {
            anchors.centerIn: parent
            width: Math.min(parent.width - Tokens.spaceXl * 2, Math.round(420 * root._s))
            height: promptColumn.implicitHeight + Tokens.spaceXl * 2
            radius: Tokens.radiusLg
            color: AppPalette.card
            border.color: AppPalette.border

            MouseArea { anchors.fill: parent }

            Column {
                id: promptColumn
                anchors.fill: parent
                anchors.margins: Tokens.spaceXl
                spacing: Tokens.spaceLg

                Text {
                    width: parent.width
                    text: qsTr("The mission has unsaved changes.")
                    color: AppPalette.textStrong
                    font.pixelSize: Tokens.fontLg
                    wrapMode: Text.WordWrap
                }

                Row {
                    spacing: Tokens.spaceMd
                    KButton {
                        text: qsTr("Save")
                        onClicked: {
                            var after = closePrompt.afterAction
                            var next = after ? after : function() { root.closeNow() }
                            if (root.plan && root.plan.filePath.length) {
                                if (!root.plan.saveFile()) return
                                closePrompt.afterAction = null
                                closePrompt.visible = false
                                next()
                            } else {
                                closePrompt.visible = false
                                root.saveNew(function() { closePrompt.afterAction = null; next() })
                            }
                        }
                    }
                    KButton {
                        text: qsTr("Discard")
                        danger: true
                        onClicked: {
                            var after = closePrompt.afterAction
                            closePrompt.afterAction = null
                            closePrompt.visible = false
                            if (after) after(); else root.closeNow()
                        }
                    }
                    KButton {
                        text: qsTr("Cancel")
                        onClicked: { closePrompt.afterAction = null; closePrompt.visible = false }
                    }
                }
            }
        }
    }
}
