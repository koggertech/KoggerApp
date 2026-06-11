import QtQuick 2.15
import kqml_types 1.0

Item {
    id: root

    anchors.fill: parent
    z: ZOrder.splashOverlay
    visible: opacity > 0.001

    MouseArea {
        anchors.fill: parent
        enabled: root.visible
        acceptedButtons: Qt.AllButtons
        onPressed: function(m) { m.accepted = true }
        onReleased: function(m) { m.accepted = true }
        onWheel: function(w) { w.accepted = true }
    }

    Rectangle {
        anchors.fill: parent
        color: AppPalette.bg
    }

    Image {
        id: logo
        anchors.centerIn: parent
        source: "qrc:/icons/app/kogger_app.png"
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
        opacity: 0.0
        scale: 0.92

        readonly property int _side: Math.round(Math.min(root.width, root.height) * 0.32)
        width: Math.max(Math.round(96 * AppPalette.scale),
                        Math.min(_side, Math.round(260 * AppPalette.scale)))
        height: width
    }

    SequentialAnimation {
        running: true

        ParallelAnimation {
            NumberAnimation { target: logo; property: "opacity"; from: 0.0; to: 1.0; duration: 260; easing.type: Easing.OutCubic }
            NumberAnimation { target: logo; property: "scale"; from: 0.92; to: 1.0; duration: 340; easing.type: Easing.OutCubic }
        }

        PauseAnimation { duration: 520 }

        NumberAnimation { target: root; property: "opacity"; to: 0.0; duration: 420; easing.type: Easing.OutCubic }
    }
}
