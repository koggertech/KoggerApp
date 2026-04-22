import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import kqml_types 1.0

Item {
    id: panelRoot
    z: 400

    property bool open: false
    property bool dimEnabled: true
    property string title: qsTr("Settings")
    property string side: "left"
    property string gearMode: "app"
    property color headerColor: AppPalette.headerBg
    property color panelColor: AppPalette.bg
    property bool panelShadowEnabled: true
    property real panelShadowOpacity: 0.72
    property int panelShadowSize: 30
    property int panelSizePx: 300
    property int scrollBarReservePx: 14
    readonly property string resolvedSide: side === "right" ? "right" : "left"
    property real progress: open ? 1.0 : 0.0
    readonly property real panelWidth: panelSizePx

    signal closeRequested()

    default property alias contentData: contentColumn.data

    visible: progress > 0.01

    Behavior on progress {
        NumberAnimation {
            duration: 220
            easing.type: Easing.InOutCubic
        }
    }

    Rectangle {
        id: shade

        anchors.fill: parent
        color: "#02061788"
        opacity: panelRoot.dimEnabled ? 0.55 * panelRoot.progress : 0.0
        z: 0
    }

    Rectangle {
        id: panel

        x: panelRoot.resolvedSide === "left"
           ? -panelRoot.panelWidth + panelRoot.panelWidth * panelRoot.progress
           : panelRoot.width - panelRoot.panelWidth * panelRoot.progress
        y: 0
        width: panelRoot.panelWidth
        height: panelRoot.height
        color: panelRoot.panelColor
        border.width: 1
        border.color: AppPalette.border
        z: 2

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            onPressed: function(mouse) { mouse.accepted = true }
            onClicked: {}
        }

        Rectangle {
            id: topSection

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            color: panelRoot.headerColor
            height: topSectionContent.y + topSectionContent.implicitHeight + 14
            z: 1

            Column {
                id: topSectionContent

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                anchors.topMargin: 14
                spacing: 12

                RowLayout {
                    width: parent.width
                    height: 46
                    spacing: 8

                    Text {
                        text: panelRoot.title
                        color: AppPalette.text
                        font.pixelSize: 18
                        font.bold: true
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                    }

                    KButton {
                        text: "\u2715"
                        width: 36
                        height: 36
                        fontPixelSize: 20
                        Layout.preferredWidth: width
                        Layout.preferredHeight: height
                        Layout.minimumWidth: width
                        Layout.minimumHeight: height
                        Layout.maximumWidth: width
                        Layout.maximumHeight: height
                        Layout.alignment: Qt.AlignVCenter
                        onClicked: panelRoot.closeRequested()
                    }
                }
            }
        }

        Flickable {
            id: contentFlick

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: topSection.bottom
            anchors.bottom: parent.bottom
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            anchors.bottomMargin: 14
            anchors.topMargin: 12

            clip: true
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick
            contentWidth: Math.max(0, width - panelRoot.scrollBarReservePx)
            contentHeight: Math.max(height, contentColumn.implicitHeight)
            interactive: contentHeight > height + 1
            z: 2

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            Column {
                id: contentColumn

                width: Math.max(0, contentFlick.width - panelRoot.scrollBarReservePx)
                spacing: 10
            }
        }
    }

    Rectangle {
        id: panelShadow

        y: panel.y
        width: panelRoot.panelShadowSize
        height: panel.height
        x: panelRoot.resolvedSide === "left" ? panel.x + panel.width : panel.x - width
        z: 1
        visible: panelRoot.panelShadowEnabled && panelRoot.progress > 0.01
        opacity: panelRoot.panelShadowOpacity * panelRoot.progress

        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop {
                position: 0.0
                color: panelRoot.resolvedSide === "left" ? AppPalette.shadow0 : "#00000000"
            }
            GradientStop {
                position: 0.45
                color: AppPalette.shadowMid
            }
            GradientStop {
                position: 1.0
                color: panelRoot.resolvedSide === "left" ? "#00000000" : AppPalette.shadow0
            }
        }
    }
}
