import QtQuick 2.15
import kqml_types 1.0

// Modal "file is opening" banner — visible while core.isFileOpening, blocks
// pointer/keyboard input so the user can't click anywhere mid-load.
Item {
    id: root

    anchors.fill: parent
    visible: typeof core !== "undefined" && core
             && core.isFileOpening
             && (typeof core.isSeparateReading === "undefined" || !core.isSeparateReading)
    z: ZOrder.fileOpeningOverlay
    enabled: visible

    // Dim layer + input blocker.
    Rectangle {
        anchors.fill: parent
        color: AppPalette.bgDeep
        opacity: 0.55
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        preventStealing: true
        acceptedButtons: Qt.AllButtons
        onPressed:  function(m) { m.accepted = true }
        onReleased: function(m) { m.accepted = true }
        onClicked:  function(m) { m.accepted = true }
        onWheel:    function(w) { w.accepted = true }
    }

    // Card with the message.
    Rectangle {
        anchors.centerIn: parent
        width: messageText.implicitWidth + 56
        height: messageText.implicitHeight + 36
        radius: 10
        color: AppPalette.card
        border.width: 1
        border.color: AppPalette.border

        Text {
            id: messageText
            anchors.centerIn: parent
            text: qsTr("Please wait, the file is opening")
            color: AppPalette.text
            font.pixelSize: 16
        }
    }
}
