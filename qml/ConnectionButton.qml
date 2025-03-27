import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

CButton {
    id: connectionButton
    property bool connection: false
    implicitWidth: implicitHeight + 3

    function openConnection() {
        if(connectionTypeCombo.currentText === qsTr("Serial")) {
            core.openConnectionAsSerial(1, autoconnectionCheck.checked, portCombo.currentText, Number(baudrateCombo.currentText), false)
        } else if(connectionTypeCombo.currentText === qsTr("IP")) {
            core.openConnectionAsIP(1, autoconnectionCheck.checked, ipAddressText.text, Number(ipPortText.text), ipTypeCombo.currentText === qsTr("TCP"));
        }
    }

    text: ""

    onClicked: {
        if(connection) {
            core.closeConnection()
        } else {
            connectionButton.openConnection()
        }
    }

    Component.onCompleted: {

    }

    onConnectionChanged: {
        canvas.requestPaint()
    }

    indicator: Canvas {
        id: canvas
        x: connectionButton.width - width - connectionButton.rightPadding
        y: connectionButton.topPadding + (connectionButton.availableHeight - height) / 2
        width: connectionButton.availableWidth
        height: connectionButton.availableHeight
        contextType: "2d"

        Connections {
            target: connectionButton

            function onPressedChanged() {
                canvas.requestPaint()
            }
        }

        onPaint: {
            context.reset();

            if(connectionButton.connection) {
                context.moveTo(0, 0);
                context.lineTo(width, 0);
                context.lineTo(width, height);
                context.lineTo(0, height);
                context.closePath();
            } else {
                context.moveTo(0, 0);
                context.lineTo(width, height/2);
                context.lineTo(0, height);
                context.closePath();
            }

            context.fillStyle = connectionButton.connection ? "#E05040" : "#40E050"
            context.fill();
        }
    }
}
