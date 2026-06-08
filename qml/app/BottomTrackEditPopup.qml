import QtQuick 2.15
import kqml_types 1.0

// Floating bottom-track tool palette. Reuses BasePanePopup's drag / snap /
// collision / position-persistence, but renders as the narrow pill column
// (transparent frame; no resize / collapse / fullscreen). Header KDragBar is
// the drag handle; content = close button + tool buttons.
BasePanePopup {
    id: root

    required property var store

    readonly property real _s: 1.5 * (theme ? theme.resCoeff : 1.0)
    readonly property int _controlH: Math.round(36 * _s) - 2
    readonly property int _sidePad: Math.round(3 * _s)
    readonly property int _gap: Math.round(6 * _s)

    readonly property var btTools: [
        { tool: 0, icon: "qrc:/icons/ui/direction_arrows.svg",  tip: qsTr("Navigate") },
        { tool: 1, icon: "qrc:/icons/ui/pencil.svg",            tip: qsTr("Draw bottom track") },
        { tool: 2, icon: "qrc:/icons/ui/arrow_bar_to_up.svg",   tip: qsTr("Raise bottom track") },
        { tool: 3, icon: "qrc:/icons/ui/arrow_bar_to_down.svg", tip: qsTr("Lower bottom track") },
        { tool: 4, icon: "qrc:/icons/ui/eraser.svg",            tip: qsTr("Erase bottom track") }
    ]
    readonly property int _btTool: (typeof core !== "undefined" && core) ? core.bottomTrackEditTool : 0

    readonly property real _pillW: _controlH + _sidePad * 2
    readonly property real _pillH: (btTools.length + 1) * _controlH + btTools.length * _gap + _sidePad * 2
    readonly property real _wantW: _pillW + contentPadding * 2
    readonly property real _wantH: headerHeight + _pillH + contentPadding

    popupVisible: store.bottomTrackEditorOpen
    dragEnabled: true
    resizeEnabled: false
    collapseButtonVisible: false
    fullscreenMode: false
    panelColor: "transparent"
    panelBorderColor: "transparent"
    headerDragBarLength: Math.max(Math.round(24 * _s), _pillW - _sidePad * 2)
    siblingSnapAlignTop: true

    function _applySize() {
        expandedWidth = _wantW
        expandedHeight = _wantH
    }

    function syncFromStore() {
        if (!popupVisible)
            return
        suspendSignals = true
        var p = store.btEditPopupPosition(popupWidth, popupHeight)
        panelX = clampX(p.x)
        panelY = clampY(p.y)
        suspendSignals = false
    }

    on_WantWChanged: _applySize()
    on_WantHChanged: _applySize()

    Component.onCompleted: {
        _applySize()
        syncFromStore()
        Qt.callLater(syncFromStore)
    }

    onPopupVisibleChanged: {
        if (popupVisible) {
            _applySize()
            syncFromStore()
            Qt.callLater(syncFromStore)
        } else if (typeof core !== "undefined" && core) {
            core.setBottomTrackEditTool(0)   // closing resets to navigation
        }
    }

    onPositionCommitted: function(x, y, w, h) {
        if (popupVisible)
            store.setBtEditPopupPosition(x, y, w, h)
    }

    onCloseRequested: store.bottomTrackEditorOpen = false

    // ── Content pill (fills the content area below the header drag bar) ──
    Rectangle {
        id: pill
        anchors.fill: parent
        radius: width / 2
        color: AppPalette.bg
        border.width: 1
        border.color: AppPalette.border

        Column {
            anchors.horizontalCenter: parent.horizontalCenter
            y: root._sidePad
            spacing: root._gap

            KCircleIconButton {
                width: root._controlH
                height: root._controlH
                iconSource: "qrc:/icons/ui/x.svg"
                iconTintColor: AppPalette.text
                toolTipText: qsTr("Close")
                fillColor:        AppPalette.card
                fillHoverColor:   AppPalette.cardHover
                fillPressedColor: AppPalette.bgDeep
                borderColor:      AppPalette.border
                borderHoverColor: AppPalette.borderHover
                onClicked: root.store.bottomTrackEditorOpen = false
            }

            Repeater {
                model: root.btTools
                delegate: KCircleIconButton {
                    required property var modelData
                    readonly property bool _sel: root._btTool === modelData.tool
                    width: root._controlH
                    height: root._controlH
                    iconSource: modelData.icon
                    iconTintColor: AppPalette.text
                    toolTipText: modelData.tip
                    fillColor:        _sel ? AppPalette.accentBgStrong : AppPalette.card
                    fillHoverColor:   _sel ? AppPalette.accentBorder : AppPalette.cardHover
                    fillPressedColor: AppPalette.bgDeep
                    borderColor:      _sel ? AppPalette.accentBorder : AppPalette.border
                    borderHoverColor: _sel ? AppPalette.accentBorder : AppPalette.borderHover
                    onClicked: {
                        if (typeof core !== "undefined" && core)
                            core.setBottomTrackEditTool(modelData.tool)
                    }
                }
            }
        }
    }
}
