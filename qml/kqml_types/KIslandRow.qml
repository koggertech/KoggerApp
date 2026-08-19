import QtQuick          // unversioned → Qt 6.8 Rectangle (per-corner radius)

Item {
    id: row

    property string label: ""
    property string caption: ""
    property color labelColor: AppPalette.textStrong
    property int labelPixelSize: island ? island.labelPixelSize : Tokens.fontLg
    property int labelElide: Text.ElideRight
    property bool stacked: false
    property bool open: true
    property bool interactive: false
    property bool chevron: false
    property bool sectionHeader: false
    property bool showSeparator: true
    property bool forceSeparator: false
    property real slotWidth: island ? island.slotWidth : 0
    property real minHeight: Math.round(44 * AppPalette.scale)
    property string toolTipText: ""

    signal clicked()

    default property alias content: slot.data

    readonly property Item island: {
        var p = parent
        while (p) {
            if (p.isKIsland === true)
                return p
            p = p.parent
        }
        return null
    }

    readonly property bool insideSection: {
        if (sectionHeader || !parent)
            return false
        var siblings = parent.children
        for (var i = 0; i < siblings.length; ++i) {
            var s = siblings[i]
            if (s === row || !s.visible || s.sectionHeader !== true)
                continue
            if (s.y < row.y)
                return true
        }
        return false
    }

    property real horizontalPadding: island ? island.rowPadding : Tokens.spaceLg
    property real verticalPadding: Tokens.spaceMd
    property real contentSpacing: Tokens.spaceSm
    property real separatorInset: island ? island.separatorInset : horizontalPadding

    readonly property real cornerRadius: island ? island.cornerRadius : 0
    readonly property bool firstInIsland: y <= 0.5
    readonly property bool lastInIsland: parent ? (y + height >= parent.height - 0.5) : false
    readonly property real topCornerRadius: firstInIsland ? cornerRadius : 0
    readonly property real bottomCornerRadius: lastInIsland ? cornerRadius : 0

    readonly property real chevronSize: Math.round(9 * AppPalette.scale)
    readonly property real trailingWidth: chevron ? chevronSize + Tokens.spaceMd : 0
    readonly property real innerWidth: Math.max(0, width - horizontalPadding * 2)
    readonly property real labelWidth: stacked
        ? innerWidth
        : Math.max(0, innerWidth - trailingWidth - slot.width - (slot.width > 0 ? Tokens.spaceMd : 0))
    readonly property real _labelH: labelBlock.implicitHeight
    readonly property real contentHeight: stacked
        ? verticalPadding * 2 + _labelH + (_labelH > 0 ? contentSpacing : 0) + slot.height
        : Math.max(minHeight, verticalPadding * 2 + Math.max(_labelH, slot.height))

    property bool _animReady: false
    Timer { interval: 0; running: true; onTriggered: row._animReady = true }

    width: parent ? parent.width : implicitWidth
    implicitHeight: contentHeight
    height: open ? contentHeight : 0
    enabled: open
    clip: height + 0.5 < contentHeight

    activeFocusOnTab: interactive && open
    Keys.onReturnPressed: row._activate()
    Keys.onEnterPressed:  row._activate()
    Keys.onSpacePressed:  row._activate()

    function _activate() {
        if (row.interactive)
            row.clicked()
    }

    KFocusRing {
        id: focusRing
        topLeftRadius: row.topCornerRadius
        topRightRadius: row.topCornerRadius
        bottomLeftRadius: row.bottomCornerRadius
        bottomRightRadius: row.bottomCornerRadius
    }

    Behavior on height {
        enabled: row._animReady
        NumberAnimation { duration: Anim.disclosureMs; easing.type: Anim.disclosureEasing }
    }

    Rectangle {
        anchors.fill: parent
        color: rowMouse.pressed ? AppPalette.bgHover : AppPalette.cardHover
        opacity: (row.interactive && (rowMouse.containsMouse || rowMouse.pressed)) ? 1 : 0
        visible: opacity > 0
        topLeftRadius: row.topCornerRadius
        topRightRadius: row.topCornerRadius
        bottomLeftRadius: row.bottomCornerRadius
        bottomRightRadius: row.bottomCornerRadius
        Behavior on opacity { NumberAnimation { duration: Anim.controlMs; easing.type: Anim.controlEasing } }
    }

    Rectangle {
        x: row.separatorInset
        width: Math.max(0, row.width - row.separatorInset * 2)
        height: Math.max(1, Math.round(AppPalette.scale))
        color: row.island ? row.island.separatorColor : AppPalette.separator
        visible: row.showSeparator && row.y > 0
                 && (row.forceSeparator || !row.insideSection)
                 && (!row.island || row.island.separatorsVisible)
    }

    MouseArea {
        id: rowMouse
        anchors.fill: parent
        enabled: row.interactive || row.toolTipText.length > 0
        hoverEnabled: enabled
        acceptedButtons: row.interactive ? Qt.LeftButton : Qt.NoButton
        cursorShape: row.interactive ? Qt.PointingHandCursor : Qt.ArrowCursor
        z: -1
        onPressed: focusRing.suppress()
        onClicked: { row.forceActiveFocus(); row.clicked() }
    }

    Column {
        id: labelBlock
        x: row.horizontalPadding
        y: row.stacked ? row.verticalPadding
                       : Math.round((row.contentHeight - implicitHeight) / 2)
        width: row.labelWidth
        spacing: Tokens.spaceXxs

        Text {
            width: parent.width
            visible: text.length > 0
            text: row.label
            color: row.labelColor
            font.pixelSize: row.labelPixelSize
            elide: row.labelElide
        }

        Text {
            width: parent.width
            visible: text.length > 0
            text: row.caption
            color: AppPalette.textSecond
            font.pixelSize: Tokens.fontSm
            wrapMode: Text.Wrap
        }
    }

    Item {
        id: slot
        width: row.stacked ? row.innerWidth
                           : (row.slotWidth > 0 ? row.slotWidth : childrenRect.width)
        height: childrenRect.height
        x: row.stacked ? row.horizontalPadding
                       : Math.max(row.horizontalPadding, row.width - row.horizontalPadding - row.trailingWidth - width)
        y: row.stacked ? labelBlock.y + row._labelH + (row._labelH > 0 ? row.contentSpacing : 0)
                       : Math.round((row.contentHeight - height) / 2)
    }

    Canvas {
        id: chevronCanvas
        visible: row.chevron
        antialiasing: true
        width: row.chevronSize
        height: Math.round(row.chevronSize * 1.7)
        x: row.width - row.horizontalPadding - width
        y: Math.round((row.contentHeight - height) / 2)

        readonly property color inkColor: AppPalette.textMuted
        onInkColorChanged: requestPaint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onVisibleChanged: if (visible) requestPaint()
        Component.onCompleted: requestPaint()

        onPaint: {
            var ctx = getContext("2d")
            if (!ctx)
                return
            ctx.clearRect(0, 0, width, height)
            ctx.strokeStyle = inkColor
            ctx.lineWidth = Math.max(1.5, Math.round(2 * AppPalette.scale))
            ctx.lineCap = "round"
            ctx.lineJoin = "round"
            ctx.beginPath()
            ctx.moveTo(width * 0.2, height * 0.12)
            ctx.lineTo(width * 0.8, height * 0.5)
            ctx.lineTo(width * 0.2, height * 0.88)
            ctx.stroke()
        }
    }

    KToolTip {
        text: row.toolTipText
        targetItem: row
        shown: rowMouse.containsMouse && row.toolTipText.length > 0
    }
}
