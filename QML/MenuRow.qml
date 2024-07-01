import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

RowLayout {
    // Layout.fillWidth: true
    width: parent.width - (Layout.leftMargin + Layout.rightMargin)
    Layout.maximumWidth: parent.width - (Layout.leftMargin + Layout.rightMargin)
    Layout.margins: 10
    Layout.topMargin: 4
    Layout.bottomMargin: 4
    spacing: 10
}
