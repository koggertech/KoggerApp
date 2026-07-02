import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import kqml_types 1.0

Column {
    id: root

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg
    readonly property real groupWidth: Math.max(0, width)

    SettingsGroup {
        id: connGroup
        width: root.groupWidth
        preferredWidth: root.groupWidth
        title: qsTr("Connections")
        description: qsTr("Connections, logging and device settings.")
        stateStore: root.store
        stateKey: "app.connections"
        collapsedByDefault: false
        contentSpacing: Tokens.spaceMd

        headerActions: KCircleIconButton {
            readonly property bool _rec: typeof core !== "undefined" && core && (core.loggingKlf || core.loggingCsv)
            width: connGroup.headerActionSize
            height: connGroup.headerActionSize
            cornerRadius: Tokens.radiusLg   // uniform rounded chip, full header height
            borderWidth: 0
            scaleOnHover: false
            glyph: "REC"
            glyphPixelSize: Math.round(width * 0.32)
            glyphColor:  _rec ? "#FCA5A5" : AppPalette.textSecond
            toolTipText: _rec ? qsTr("Stop recording") : qsTr("Start recording")
            fillColor:      _rec ? "#7F1D1D" : AppPalette.controlRaised
            fillHoverColor: _rec ? "#991B1B" : Qt.lighter(AppPalette.controlRaised, 1.2)
            onClicked: if (root.store) root.store.setRecording(!root.store.isRecording)
        }

        Loader {
            id: connectionsLoader
            width: parent.width
            active: connGroup.expanded
                    && (root.store ? root.store.settingsPanelOpen === true : false)
            asynchronous: true
            source: "qrc:/qml/devices/ConnectionViewer.qml"

            onLoaded: {
                if (item) {
                    item.width = width
                    item.store = root.store
                }
            }

            onWidthChanged: {
                if (item)
                    item.width = width
            }
        }
    }
}
