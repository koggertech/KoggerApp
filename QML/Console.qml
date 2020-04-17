import QtQuick 2.6
import QtQuick.Controls 2.4

Rectangle {
    id: consoleOut
    width: parent.width
    height: parent.height/8
    y: parent.height - height
    color: "#D0D0D0"

    ListView {
        anchors.fill: parent
        model: core.consoleList

        ScrollBar.vertical: ScrollBar { }

        ListModel {
             id: colorCategory

             ListElement {
                 name: "QtDebugMsg"
                 color: "gray"
             }
             ListElement {
                 name: "QtWarningMsg"
                 color: "orange"
             }
             ListElement {
                 name: "QtCriticalMsg"
                 color: "red"
             }
             ListElement {
                 name: "QtFatalMsg"
                 color: "red"
             }
             ListElement {
                 name: "QtInfoMsg"
                 color: "LimeGreen"
             }
         }

        delegate: TextEdit  {
            textFormat: TextEdit.RichText
            text: time + ":    " + payload
            height: 15
            color: colorCategory.get(category).color

            wrapMode: Text.WordWrap
            readOnly: true
            selectByMouse: true
        }
        focus: true

    }
}
