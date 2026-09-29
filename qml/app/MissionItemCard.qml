import QtQuick 2.15
import kqml_types 1.0

Rectangle {
    id: card

    property var editor: null
    property string itemId: ""
    property int ordinal: 0
    property bool isRally: false

    readonly property var d: editor ? editor.itemData(itemId) : null
    readonly property var info: editor ? editor.itemInfo(itemId) : ({})
    readonly property string type: d ? String(d.type) : ""
    readonly property bool selected: editor && editor.ctl && editor.ctl.selectedId === itemId
    readonly property bool shapeError: (type === "survey" || type === "corridor") && !!(info.error && info.error.length)
    readonly property real _s: AppPalette.scale
    readonly property int spinW: Math.round(110 * _s)

    property Item dragLayer: null
    property int visualIndex: ordinal - 1
    readonly property int handleW: Math.round(28 * _s)
    readonly property bool isPoint: type === "waypoint" || isRally
    readonly property int actionsW: (isRally ? 1 : 3) * (Tokens.controlHMd + Tokens.spaceXs) + Tokens.spaceMd
    readonly property int actionsRightMargin: isRally ? 0 : handleW + Tokens.spaceMd
    readonly property bool dragActive: dragArea.drag.active
    readonly property color typeColor: shapeError ? AppPalette.dangerBorder
                                     : isRally ? AppPalette.missionRally
                                     : type === "survey" ? AppPalette.missionSurvey
                                     : type === "corridor" ? AppPalette.missionCorridor
                                     : AppPalette.missionWaypoint

    component HeaderButton: KCircleIconButton {
        width: Tokens.controlHMd
        height: Tokens.controlHMd
        hitPadding: Math.round(6 * card._s)
        iconPixelSize: Tokens.iconSm
        iconTintColor: enabled ? (card.selected ? AppPalette.accentText : AppPalette.text) : AppPalette.textMuted
        fillColor: card.selected ? Qt.rgba(0, 0, 0, 0.18) : AppPalette.card
        fillHoverColor: card.selected ? Qt.rgba(0, 0, 0, 0.28) : AppPalette.cardHover
        borderColor: card.selected ? Qt.rgba(0, 0, 0, 0.25) : AppPalette.border
        borderWidth: 1
    }

    component FieldLabel: Text {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        color: AppPalette.accentText
        font.pixelSize: Tokens.fontSm
    }

    signal dragStarted()
    signal dragMoved()
    signal dragFinished()

    height: column.implicitHeight + Tokens.spaceMd * 2
    radius: Tokens.radiusMd
    color: dragActive ? AppPalette.cardHover : (selected ? AppPalette.accentBg : AppPalette.rowRaised)
    border.color: selected ? AppPalette.accentBorder : (shapeError ? AppPalette.dangerBorder : (dragActive ? AppPalette.border : "transparent"))
    border.width: (selected || dragActive || shapeError) ? 1 : 0
    scale: dragActive ? 1.02 : 1.0
    z: dragActive ? 10 : 0

    Behavior on scale { NumberAnimation { duration: Anim.controlMs; easing.type: Anim.controlEasing } }

    states: State {
        when: card.dragActive && card.dragLayer !== null
        ParentChange { target: card; parent: card.dragLayer }
    }

    onDragActiveChanged: {
        if (dragActive) dragStarted()
        else {
            dragFinished()
            Qt.callLater(function() { if (!card.dragActive && card.dragLayer !== null) { card.x = 0; card.y = 0 } })
        }
    }

    function fmtCoord(v, digits) { return isFinite(v) ? Number(v).toFixed(digits) : "—" }
    function coordString(digits) { return d ? fmtCoord(d.lat, digits) + ", " + fmtCoord(d.lon, digits) : "" }
    function copyCoords() {
        if (typeof core !== "undefined" && core) core.copyToClipboard(coordString(8))
        if (typeof notifications !== "undefined" && notifications) notifications.info(qsTr("Coordinates copied"))
    }

    function explicitEntry() {
        if (!d) return null
        var v = type === "survey" ? d.entryCorner : d.entryEnd
        return (v === undefined) ? null : v
    }

    function patch(obj) {
        if (editor) editor.patch(itemId, obj)
    }

    function beginEdit() {
        if (editor && editor.plan) editor.plan.beginTransaction()
    }

    function endEdit() {
        if (editor && editor.plan) editor.plan.endTransaction(true)
    }

    Item {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: headerRow.height + Tokens.spaceMd * 2

        KTapArea {
            anchors.fill: parent
            cursorPointing: true
            onTapped: {
                if (!card.editor || !card.editor.ctl) return
                if (card.selected) card.editor.ctl.clearSelection()
                else card.editor.ctl.select(card.itemId, -1)
            }
        }
    }

    Column {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Tokens.spaceMd
        spacing: Tokens.spaceSm

        Row {
            id: headerRow
            width: parent.width
            spacing: Tokens.spaceMd

            Rectangle {
                width: Tokens.controlHSm
                height: Tokens.controlHSm
                radius: width / 2
                color: card.typeColor
                Text {
                    anchors.centerIn: parent
                    text: card.isRally ? "R" : String(card.ordinal)
                    color: AppPalette.textOn(card.typeColor)
                    font.pixelSize: Tokens.fontSm
                    font.bold: true
                }
            }

            Column {
                width: parent.width - Tokens.controlHSm - Tokens.spaceMd - card.actionsW - (card.isRally ? 0 : card.handleW + Tokens.spaceMd)
                Text {
                    text: card.editor ? card.editor.typeLabel(card.isRally ? "rally" : card.type) : card.type
                    color: card.selected ? AppPalette.accentText : AppPalette.textStrong
                    font.pixelSize: Tokens.fontBase
                    font.bold: true
                }
                Text {
                    width: parent.width
                    visible: text.length > 0
                    elide: Text.ElideRight
                    color: card.selected ? AppPalette.accentText : (card.shapeError ? AppPalette.linkDownText : AppPalette.textSecond)
                    font.pixelSize: Tokens.fontSm
                    text: !card.d ? ""
                        : card.isPoint ? (card.selected ? "" : card.coordString(6))
                        : card.shapeError ? qsTr("Not generated")
                        : qsTr("%1 lines, %2 waypoints").arg(card.info.lineCount || 0).arg(card.info.waypointCount || 0)
                }
            }

            Row {
                id: headerActions
                spacing: Tokens.spaceXs
                anchors.verticalCenter: parent.verticalCenter

                HeaderButton {
                    visible: !card.isRally
                    iconSource: "qrc:/icons/ui/chevron-up.svg"
                    toolTipText: qsTr("Move up")
                    enabled: card.ordinal > 1
                    onClicked: if (card.editor) card.editor.moveRouteItem(card.itemId, -1)
                }
                HeaderButton {
                    visible: !card.isRally
                    iconSource: "qrc:/icons/ui/chevron-down.svg"
                    toolTipText: qsTr("Move down")
                    enabled: card.editor && card.ordinal < card.editor.itemIds.length
                    onClicked: if (card.editor) card.editor.moveRouteItem(card.itemId, 1)
                }
                HeaderButton {
                    iconSource: "qrc:/icons/ui/x.svg"
                    iconTintColor: card.selected ? AppPalette.accentText : AppPalette.dangerText
                    fillColor: card.selected ? Qt.rgba(0, 0, 0, 0.18) : AppPalette.dangerBg
                    fillHoverColor: card.selected ? Qt.rgba(0, 0, 0, 0.28) : AppPalette.dangerHover
                    borderColor: card.selected ? Qt.rgba(0, 0, 0, 0.25) : AppPalette.dangerBorder
                    toolTipText: qsTr("Delete")
                    onClicked: if (card.editor && card.editor.plan) card.editor.plan.removeItem(card.itemId)
                }
            }

            Item {
                id: dragHandle
                visible: !card.isRally
                width: card.handleW
                height: Tokens.controlHMd
                anchors.verticalCenter: parent.verticalCenter

                Column {
                    anchors.centerIn: parent
                    spacing: Math.round(3 * card._s)
                    Repeater {
                        model: 3
                        Rectangle {
                            width: Math.round(14 * card._s)
                            height: Math.max(2, Math.round(2 * card._s))
                            radius: height / 2
                            color: card.dragActive ? AppPalette.accentBar : (card.selected ? AppPalette.accentText : AppPalette.textMuted)
                        }
                    }
                }

                MouseArea {
                    id: dragArea
                    anchors.fill: parent
                    anchors.margins: -Tokens.spaceXs
                    hoverEnabled: true
                    preventStealing: true
                    cursorShape: Qt.SizeVerCursor
                    drag.target: card
                    drag.axis: Drag.YAxis
                    drag.minimumY: 0
                    drag.maximumY: card.dragLayer ? Math.max(0, card.dragLayer.height - card.height) : 0
                    onPositionChanged: if (drag.active) card.dragMoved()
                }
            }
        }

        Rectangle {
            visible: card.shapeError && !card.dragActive && !(card.editor && card.editor.draggingItemId.length > 0)
            width: parent.width
            height: errorText.implicitHeight + Tokens.spaceSm * 2
            radius: Tokens.radiusSm
            color: AppPalette.dangerBg
            border.color: AppPalette.dangerBorder
            border.width: 1

            Text {
                id: errorText
                anchors.fill: parent
                anchors.margins: Tokens.spaceSm
                anchors.leftMargin: Tokens.spaceMd
                text: card.info.error || ""
                wrapMode: Text.WordWrap
                color: AppPalette.dangerText
                font.pixelSize: Tokens.fontSm
                verticalAlignment: Text.AlignVCenter
            }
        }

        Loader {
            id: editorLoader
            width: parent.width
            active: card.selected && card.d !== null && !card.dragActive && !(card.editor && card.editor.draggingItemId.length > 0)
            visible: active
            sourceComponent: editorComponent
        }
    }

    Component {
        id: editorComponent

        Column {
            width: editorLoader.width
            spacing: Tokens.spaceSm

            Item {
                width: parent.width
                height: Tokens.controlHMd
                Text {
                    id: coordText
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.min(implicitWidth, parent.width - (Tokens.controlHMd + Tokens.spaceSm) * (card.isPoint ? 2 : 1))
                    elide: Text.ElideRight
                    color: AppPalette.accentText
                    font.pixelSize: Tokens.fontSm
                    text: card.isPoint ? card.coordString(6)
                        : card.d && card.type === "survey" ? qsTr("%1 corners").arg(card.d.polygon ? card.d.polygon.length : 0)
                        : card.d ? qsTr("%1 axis points").arg(card.d.axis ? card.d.axis.length : 0) : ""
                }
                HeaderButton {
                    id: copyBtn
                    visible: card.isPoint
                    anchors.left: coordText.right
                    anchors.leftMargin: Tokens.spaceSm
                    anchors.verticalCenter: parent.verticalCenter
                    iconSource: "qrc:/icons/ui/copy.svg"
                    toolTipText: qsTr("Copy coordinates")
                    onClicked: card.copyCoords()
                }
                HeaderButton {
                    anchors.left: card.isPoint ? copyBtn.right : coordText.right
                    anchors.leftMargin: Tokens.spaceSm
                    anchors.verticalCenter: parent.verticalCenter
                    iconSource: "qrc:/icons/ui/current-location.svg"
                    toolTipText: qsTr("Show on map")
                    onClicked: if (card.editor && card.editor.ctl) card.editor.ctl.showItem(card.itemId)
                }
            }

            Item {
                visible: card.type === "waypoint"
                width: parent.width
                height: Tokens.controlHMd
                FieldLabel { text: qsTr("Hold, s") }
                KSpinBox {
                    id: holdSpin
                    anchors.right: parent.right
                    anchors.rightMargin: card.actionsRightMargin
                    anchors.verticalCenter: parent.verticalCenter
                    width: card.spinW; height: Tokens.controlHMd
                    from: 0; to: 3600; stepSize: 1
                    onValueModified: function(v) { card.patch({ holdTime: v }) }
                }
                Binding {
                    target: holdSpin
                    property: "value"
                    value: card.d && card.type === "waypoint" ? Math.round(card.d.holdTime || 0) : 0
                }
            }
            Item {
                visible: card.type === "waypoint"
                width: parent.width
                height: Tokens.controlHMd
                FieldLabel { text: qsTr("Speed, m/s") }
                KSpinBox {
                    id: speedSpin
                    anchors.right: parent.right
                    anchors.rightMargin: card.actionsRightMargin
                    anchors.verticalCenter: parent.verticalCenter
                    width: card.spinW; height: Tokens.controlHMd
                    from: 0; to: 300; stepSize: 1; divisor: 10; decimals: 1
                    toolTipText: qsTr("0 = plan cruise speed")
                    onValueModified: function(v) { card.patch({ speed: v > 0 ? v / 10 : null }) }
                }
                Binding {
                    target: speedSpin
                    property: "value"
                    value: card.d && card.type === "waypoint" && card.d.speed !== null && card.d.speed !== undefined ? Math.round(card.d.speed * 10) : 0
                }
            }

            MissionSliderRow {
                visible: card.type === "survey" || card.type === "corridor"
                width: parent.width
                label: qsTr("Line spacing")
                unit: qsTr("m")
                from: 1; to: 30; stepSize: 0.5; decimals: 1
                defaultValue: 10
                value: card.d && card.d.lineSpacing !== undefined ? card.d.lineSpacing : 10
                onLiveChanged: function(v) { card.patch({ lineSpacing: v }) }
                onEditBegan: card.beginEdit()
                onEditEnded: card.endEdit()
            }
            Row {
                visible: card.type === "survey"
                spacing: Tokens.spaceMd
                width: parent.width
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Angle, °")
                    color: AppPalette.accentText
                    font.pixelSize: Tokens.fontSm
                    width: Math.round(90 * card._s)
                    KTapArea { onDoubleTapped: card.patch({ angleDeg: 0 }) }
                }
                MissionAngleDial {
                    angle: card.d && card.d.angleDeg !== undefined ? card.d.angleDeg : 0
                    onPressed: card.beginEdit()
                    onMoved: function(deg) { card.patch({ angleDeg: deg }) }
                    onReleased: function(deg) {
                        card.patch({ angleDeg: deg })
                        card.endEdit()
                    }
                }
            }
            MissionSliderRow {
                visible: card.type === "corridor"
                width: parent.width
                label: qsTr("Width")
                unit: qsTr("m")
                from: 2; to: 500; stepSize: 1; decimals: 0
                defaultValue: 40
                value: card.d && card.d.width !== undefined ? card.d.width : 40
                onLiveChanged: function(v) { card.patch({ width: v }) }
                onEditBegan: card.beginEdit()
                onEditEnded: card.endEdit()
            }
            MissionSliderRow {
                visible: card.type === "survey" || card.type === "corridor"
                width: parent.width
                label: qsTr("Turnaround")
                unit: qsTr("m")
                from: -20; to: 20; stepSize: 0.5; decimals: 1
                defaultValue: 0
                value: card.d && card.d.turnaround !== undefined ? card.d.turnaround : 0
                onLiveChanged: function(v) { card.patch({ turnaround: v }) }
                onEditBegan: card.beginEdit()
                onEditEnded: card.endEdit()
            }
            Row {
                visible: card.type === "survey" || card.type === "corridor"
                spacing: Tokens.spaceMd
                width: parent.width
                Text { anchors.verticalCenter: parent.verticalCenter; text: qsTr("Entry"); color: AppPalette.accentText; font.pixelSize: Tokens.fontSm; width: Math.round(90 * card._s) }
                KButton {
                    height: Tokens.controlHMd
                    text: {
                        var explicit = card.explicitEntry()
                        var resolved = card.info.resolvedEntry !== undefined ? card.info.resolvedEntry : 0
                        return (explicit === null ? qsTr("auto") : qsTr("manual")) + " · " + (resolved + 1)
                    }
                    onClicked: {
                        var count = 4
                        var resolved = card.info.resolvedEntry !== undefined ? card.info.resolvedEntry : 0
                        var next = (resolved + 1) % count
                        if (card.type === "survey") card.patch({ entryCorner: next })
                        else card.patch({ entryEnd: next })
                    }
                }
                KButton {
                    height: Tokens.controlHMd
                    text: qsTr("Auto")
                    visible: card.explicitEntry() !== null
                    onClicked: {
                        if (card.type === "survey") card.patch({ entryCorner: null })
                        else card.patch({ entryEnd: null })
                    }
                }
            }
            KSwitch {
                visible: card.type === "survey"
                flat: true
                width: parent.width
                text: qsTr("Crosshatch")
                checked: card.d && card.d.crosshatch === true
                onToggled: card.patch({ crosshatch: checked })
            }
            Text {
                visible: card.type === "survey" || card.type === "corridor"
                width: parent.width
                wrapMode: Text.WordWrap
                color: AppPalette.accentText
                font.pixelSize: Tokens.fontSm
                text: (card.info.error && card.info.error.length) ? card.info.error
                    : qsTr("Length %1 m").arg(Math.round(card.info.lengthMeters || 0))
                      + (card.type === "survey" ? qsTr(", area %1 ha").arg(((card.info.areaSquareMeters || 0) / 10000).toFixed(2)) : "")
                      + (card.info.spacingClamped ? qsTr(", spacing raised to %1 m").arg((card.info.effectiveSpacing || 0).toFixed(1)) : "")
            }
        }
    }
}
