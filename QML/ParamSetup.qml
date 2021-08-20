import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12

RowLayout {
    Layout.fillWidth: true
    spacing: 10

    property string paramName: "Param"

    CText {
        Layout.fillWidth: true
        text: paramName
    }
}
