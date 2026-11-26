import QtQuick 2.15
import kqml_types 1.0

Item {
    id: bar

    required property BasePanePopup popup
    property string title: ""
    property real titleInset: popup ? popup.contentPadding : 0

    signal settingsClicked()
    signal closeClicked()

    readonly property real _pad: popup ? popup.contentPadding : 0
    readonly property real _btn: Math.max(Math.round(18 * AppPalette.scale), height - _pad * 2)

    anchors.fill: parent

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        cursorShape: Qt.OpenHandCursor
        onPressed: function(mouse) { mouse.accepted = true }
    }

    DragHandler {
        target: null
        enabled: !!bar.popup && bar.popup.dragEnabled && !bar.popup.collapsed && !bar.popup.fullscreenMode
        xAxis.enabled: true
        yAxis.enabled: true
        onActiveChanged: active ? bar.popup._beginDrag() : bar.popup._endDrag()
        onTranslationChanged: if (active) bar.popup._updateDrag(translation.x, translation.y)
    }

    Text {
        x: bar.titleInset
        anchors.verticalCenter: parent.verticalCenter
        text: bar.title
        color: AppPalette.textStrong
        font.pixelSize: Tokens.fontBase
    }

    KDragBar {
        anchors.centerIn: parent
        orientation: "horizontal"
        barColor: AppPalette.controlRaised
    }

    KCircleIconButton {
        id: closeBtn
        anchors.right: parent.right
        anchors.rightMargin: bar._pad
        anchors.top: parent.top
        anchors.topMargin: bar._pad
        width: bar._btn
        height: bar._btn
        rounded: false
        cornerRadius: Tokens.radiusLg
        z: 1
        iconSource: ""
        glyph: "×"
        showGlyphWithIcon: true
        glyphPixelSize: Tokens.iconSm
        glyphColor: AppPalette.textSecond
        fillColor: AppPalette.controlRaised
        fillHoverColor: Qt.lighter(AppPalette.controlRaised, 1.2)
        fillPressedColor: AppPalette.bgDeep
        borderColor: AppPalette.border
        borderHoverColor: AppPalette.borderHover
        toolTipText: qsTr("Hide panel")
        onClicked: bar.closeClicked()
    }

    KCircleIconButton {
        anchors.right: closeBtn.left
        anchors.rightMargin: bar._pad
        anchors.top: closeBtn.top
        width: bar._btn
        height: bar._btn
        rounded: false
        cornerRadius: Tokens.radiusLg
        z: 1
        iconSource: "qrc:/icons/ui/pencil.svg"
        iconTintColor: AppPalette.textSecond
        fillColor: AppPalette.controlRaised
        fillHoverColor: Qt.lighter(AppPalette.controlRaised, 1.2)
        fillPressedColor: AppPalette.bgDeep
        borderColor: AppPalette.border
        borderHoverColor: AppPalette.borderHover
        toolTipText: qsTr("Panel settings")
        onClicked: bar.settingsClicked()
    }
}
