import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

RowLayout {
    Layout.fillWidth: true
    Layout.leftMargin: 15
    Layout.rightMargin: 15
    Layout.topMargin: 5
    Layout.bottomMargin: 10

    property string titleText: "TitleBox"
    property bool closeble: false
    property bool isOpen: checkBoxOpen.checked || !closeble

    CCheck {
        id: checkBoxOpen
        visible: closeble
        checked: true
        text: ""
    }

    Text {
        Layout.fillWidth: true
        text: titleText
        color: "#959595"
        font.pointSize: 12
    }
}
