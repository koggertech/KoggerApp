import QtQuick 2.15
import kqml_types 1.0

Column {
    id: page

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceMd

    readonly property string body: (page.store && page.store.licenseViewFile.length > 0)
                                   ? appUtils.licenseText(page.store.licenseViewFile) : ""

    Text {
        width: parent.width
        wrapMode: Text.WordWrap
        textFormat: Text.PlainText
        text: page.body.length > 0 ? page.body
                                   : qsTr("The license text could not be read from the application resources.")
        color: AppPalette.textMuted
        font.pixelSize: Tokens.fontXs
    }
}
