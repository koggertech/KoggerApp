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
            readonly property bool _rec: typeof core !== "undefined" && core && core.loggingKlf
            width: connGroup.headerActionSize
            height: connGroup.headerActionSize
            glyph: "KLF"
            glyphPixelSize: Math.round(width * 0.34)
            glyphColor:  _rec ? "#FCA5A5" : AppPalette.textSecond
            toolTipText: _rec ? qsTr("Stop KLF logging") : qsTr("Start KLF logging")
            fillColor:   _rec ? "#7F1D1D" : AppPalette.card
            borderColor: _rec ? "#EF4444" : AppPalette.border
            onClicked: if (typeof core !== "undefined" && core) core.setKlfLogging(!core.loggingKlf)
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
