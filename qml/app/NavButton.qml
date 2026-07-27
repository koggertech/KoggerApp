import QtQuick 2.15
import QtQuick.Layouts 1.15
import kqml_types 1.0

KButton {
    id: control

    property string info: ""

    horizontalPadding: Math.round(14 * AppPalette.scale)
    verticalPadding: Math.round(7 * AppPalette.scale)
    implicitWidth: navRow.implicitWidth + horizontalPadding * 2
    implicitHeight: Math.max(Tokens.controlHMd, navRow.implicitHeight + verticalPadding * 2)

    contentItem: RowLayout {
        id: navRow
        spacing: Math.round(8 * AppPalette.scale)

        Text {
            Layout.fillWidth: true
            text: control.text
            color: control.hovered ? Qt.lighter(control.textColor, 1.08) : control.textColor
            font.pixelSize: control.fontPixelSize
            font.bold: control.bold
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter

            Behavior on color { ColorAnimation { duration: 110; easing.type: Easing.OutCubic } }
        }

        Text {
            visible: control.info.length > 0
            text: control.info
            color: AppPalette.textMuted
            font.pixelSize: Math.round(control.fontPixelSize * 0.9)
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter
            Layout.maximumWidth: Math.round(control.availableWidth * 0.5)
        }

        DisclosureIndicator {
            Layout.alignment: Qt.AlignVCenter
            implicitWidth: Math.round(10 * AppPalette.scale)
            implicitHeight: Math.round(10 * AppPalette.scale)
            expanded: false
            indicatorColor: control.hovered ? AppPalette.text : AppPalette.textSecond
        }
    }
}
