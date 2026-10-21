import QtQuick 2.15
import kqml_types 1.0
import "StatusSeverity.js" as Sev

Rectangle {
    id: banner

    property string sev: "idle"
    property string word: ""
    property string sub: ""
    property string alert: ""
    property bool pulse: false
    property bool dimmed: false
    property Component aside: null
    default property alias content: bannerCol.data

    readonly property color sevColor: Sev.color(AppPalette, banner.sev)
    readonly property bool _asideShown: !!asideLoader.item && asideLoader.item.visible

    radius: Tokens.radiusMd
    color: banner.sev === "idle" ? "transparent"
                                 : Qt.rgba(banner.sevColor.r, banner.sevColor.g, banner.sevColor.b, 0.12)
    border.width: banner.sev === "idle" ? 1 : 0
    border.color: AppPalette.border
    implicitHeight: bannerCol.implicitHeight + Tokens.spaceMd * 2
    height: implicitHeight
    opacity: banner.dimmed ? 0.6 : 1.0

    Rectangle {
        anchors.left: parent.left
        anchors.leftMargin: Math.round(2 * AppPalette.scale)
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: parent.radius
        anchors.bottomMargin: parent.radius
        width: Math.round(3 * AppPalette.scale)
        radius: width / 2
        color: banner.sevColor
        visible: banner.sev !== "idle"
    }

    Column {
        id: bannerCol
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: Tokens.spaceMd + Math.round(4 * AppPalette.scale)
        anchors.rightMargin: Tokens.spaceMd
        anchors.verticalCenter: parent.verticalCenter
        spacing: Tokens.spaceMd

        Row {
            id: heroRow
            width: parent.width
            spacing: Tokens.spaceMd

            Column {
                id: heroTextBlock
                anchors.verticalCenter: parent.verticalCenter
                width: heroRow.width - (banner._asideShown ? asideLoader.width + heroRow.spacing : 0)
                spacing: Math.round(2 * AppPalette.scale)

                Row {
                    width: parent.width
                    spacing: Tokens.spaceMd

                    Item {
                        id: heroDisc
                        width: Math.round(16 * AppPalette.scale)
                        height: width
                        anchors.verticalCenter: parent.verticalCenter

                        Rectangle {
                            id: pulseRing
                            anchors.centerIn: parent
                            width: parent.width
                            height: parent.height
                            radius: width / 2
                            color: "transparent"
                            border.width: Math.max(1, Math.round(1.5 * AppPalette.scale))
                            border.color: banner.sevColor
                            visible: banner.pulse
                            ParallelAnimation {
                                running: banner.pulse
                                loops: Animation.Infinite
                                NumberAnimation { target: pulseRing; property: "scale"; from: 1.0; to: 2.4; duration: 1500; easing.type: Easing.OutQuad }
                                NumberAnimation { target: pulseRing; property: "opacity"; from: 0.5; to: 0.0; duration: 1500; easing.type: Easing.OutQuad }
                            }
                        }
                        Rectangle {
                            anchors.centerIn: parent
                            width: parent.width
                            height: parent.height
                            radius: width / 2
                            color: banner.sevColor
                        }
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - heroDisc.width - parent.spacing
                        text: banner.word
                        color: Sev.text(AppPalette, banner.sev)
                        font.pixelSize: Tokens.fontXxl
                        font.bold: true
                        elide: Text.ElideRight
                    }
                }

                Text {
                    width: parent.width
                    visible: banner.sub.length > 0
                    text: banner.sub
                    color: AppPalette.textSecond
                    font.pixelSize: Tokens.fontSm
                    wrapMode: Text.WordWrap
                }

                Text {
                    width: parent.width
                    visible: banner.alert.length > 0
                    text: banner.alert
                    color: AppPalette.linkDownText
                    font.pixelSize: Tokens.fontSm
                    font.bold: true
                    wrapMode: Text.WordWrap
                }
            }

            Loader {
                id: asideLoader
                anchors.verticalCenter: parent.verticalCenter
                active: banner.aside !== null
                sourceComponent: banner.aside
                width: banner._asideShown ? asideLoader.item.implicitWidth : 0
                height: heroTextBlock.implicitHeight
            }
        }
    }
}
