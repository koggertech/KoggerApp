import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

RowLayout {
    Layout.fillWidth: true
    Layout.leftMargin: 15
    Layout.rightMargin: 15
    Layout.topMargin: 5
    Layout.bottomMargin: 5

    property string titleText: qsTr("TitleBox")
    property bool closeble: false
    property bool isOpen: checkBoxOpen.checked || !closeble

    CCheck {
        id: checkBoxOpen
        visible: closeble
        checked: true
        text: ""
    }

    CText {
        Layout.fillWidth: true
        text: titleText
    }
}
