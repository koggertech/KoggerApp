import QtQuick 2.15
import QtQuick.Layouts 1.15
import kqml_types 1.0

KCircleIconButton {
    id: gear
    property string modeTag: "app"

    iconSource: "qrc:/icons/ui/settings.svg"
    // Gear has more internal whitespace than other Tabler icons, so push it
    // to ~64% of the button (vs the 56% default). Scales with manualScale.
    iconPixelSize: Math.round((modeTag === "app" ? 24 : 22) * AppPalette.scale)
    toolTipText: modeTag === "3D" ? qsTr("3D settings")
                                 : modeTag === "2D" ? qsTr("2D settings")
                                                    : qsTr("Settings")
    width: Math.round(36 * AppPalette.scale)
    height: Math.round(36 * AppPalette.scale)
    padding: 0
    focusPolicy: Qt.NoFocus
    Layout.preferredWidth: width
    Layout.preferredHeight: height
    Layout.minimumWidth: width
    Layout.minimumHeight: height
    Layout.maximumWidth: width
    Layout.maximumHeight: height
    Layout.alignment: Qt.AlignVCenter
}
