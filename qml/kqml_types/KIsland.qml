import QtQuick 2.15

Item {
    id: island

    readonly property bool isKIsland: true

    property string title: ""
    property string footer: ""
    property color fillColor: AppPalette.card
    property color borderColor: AppPalette.border
    property color titleColor: AppPalette.textStrong
    property color footerColor: AppPalette.textSecond
    property color separatorColor: AppPalette.separator
    property real cornerRadius: Tokens.radiusLg
    property real rowPadding: Tokens.spaceLg
    property real titleInset: rowPadding
    property real separatorInset: rowPadding
    property real slotWidth: 0
    property real rowMinHeight: Tokens.rowH
    property int labelPixelSize: Tokens.fontLg
    property bool separatorsVisible: true

    default property alias rows: rowColumn.data

    width: parent ? parent.width : implicitWidth
    implicitWidth: Math.round(240 * AppPalette.scale)
    implicitHeight: stack.implicitHeight

    Column {
        id: stack
        width: parent.width
        spacing: Tokens.spaceSm

        Text {
            x: island.titleInset
            width: Math.max(0, parent.width - island.titleInset * 2)
            visible: island.title.length > 0
            text: island.title
            color: island.titleColor
            font.pixelSize: Tokens.fontBase
            elide: Text.ElideRight
        }

        Rectangle {
            width: parent.width
            radius: island.cornerRadius
            color: island.fillColor
            border.width: Tokens.cardBorderWidth
            border.color: island.borderColor
            implicitHeight: rowColumn.implicitHeight

            Column {
                id: rowColumn
                width: parent.width
                spacing: 0
            }
        }

        Text {
            x: island.rowPadding
            width: Math.max(0, parent.width - island.rowPadding * 2)
            visible: island.footer.length > 0
            text: island.footer
            color: island.footerColor
            font.pixelSize: Tokens.fontSm
            wrapMode: Text.Wrap
        }
    }
}
