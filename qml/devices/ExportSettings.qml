import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs
import "../controls"
import "../menus"

MenuScroll {
    id: scrollBar

    ColumnLayout {
        width: parent.width

        spacing: 10

        ExportDist {
            Layout.preferredWidth: parent.width
        }

    }
}
