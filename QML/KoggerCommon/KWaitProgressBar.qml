import QtQuick 2.0
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15

Rectangle{
    property string text : qsTr("Please, wait...")
    property string textColor : "black"

    id:      root
    objectName: ""
    visible: true
    width:   300
    height:  100
    color:   "transparent"

    ColumnLayout{
        Layout.fillHeight: true
        Layout.fillWidth:  true
        Layout.alignment:  Qt.AlignCenter
        width:             100
        height:            50
        spacing:           10

        Text{
            text:                root.text
            color:               textColor
            font:                theme.textFont
            horizontalAlignment: Text.AlignHCenter
        }

        //ProgressBar{
        //    id:               surfaceProcessingProgressBar
        //    value:            0.0
        //    indeterminate:    true
        //    Layout.fillWidth: true
        //}

        anchors.bottom:           parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }

    anchors.verticalCenter:   parent.verticalCenter;
    anchors.horizontalCenter: parent.horizontalCenter
}
