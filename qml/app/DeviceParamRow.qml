import QtQuick 2.15
import kqml_types 1.0

KIslandRow {
    id: paramRow

    property var page: null
    property string paramKey: ""
    property bool favorite: false

    readonly property var dev: page ? page.dev : null
    readonly property bool editMode: !!(page && page.favEditMode)
    readonly property bool pinned: !!page && page.favoriteKeys.indexOf(paramKey) >= 0
    readonly property bool pinnable: !!page && page.isPinnable(paramKey)

    readonly property var meta: DeviceParamCatalog.meta(paramKey)
    readonly property var paramValue: {
        var d = paramRow.dev
        var m = paramRow.meta
        if (!d || !m)
            return 0
        return DeviceParamCatalog.read(d, paramRow.paramKey)
    }

    function writeParam(v) {
        if (paramRow.dev)
            DeviceParamCatalog.write(paramRow.dev, paramRow.paramKey, v)
    }

    label: !meta ? paramKey
         : (meta.labelInFavoritesOnly === true && !favorite) ? ""
                                                             : meta.label
    stacked: !!meta && (meta.stacked === true || meta.kind === "tabs")
    verticalPadding: stacked ? Tokens.spaceMd : Tokens.spaceXs
    visible: DeviceParamCatalog.supported(dev, paramKey)

    leading: KCircleIconButton {
        readonly property real _sz: Tokens.controlHMd
        width: (paramRow.editMode && paramRow.pinnable) ? _sz : 0
        height: _sz
        visible: width > 0.5
        opacity: paramRow.editMode ? 1.0 : 0.0
        Behavior on width   { NumberAnimation { duration: Anim.fadeMs; easing.type: Easing.OutCubic } }
        Behavior on opacity { NumberAnimation { duration: Anim.fadeMs; easing.type: Easing.OutCubic } }
        rounded: false
        cornerRadius: Tokens.radiusMd
        borderWidth: 0
        iconSource: paramRow.pinned ? "qrc:/icons/ui/star-filled.svg" : "qrc:/icons/ui/star.svg"
        iconPixelSize: Math.round(_sz * 0.62)
        iconTintColor: paramRow.pinned ? AppPalette.accentText : AppPalette.textSecond
        fillColor:      paramRow.pinned ? AppPalette.accentBgStrong : AppPalette.chipRaised
        fillHoverColor: paramRow.pinned ? AppPalette.accentBgStrong : AppPalette.chipRaisedHover
        toolTipText: paramRow.pinned ? qsTr("Unpin from favourites") : qsTr("Pin to favourites")
        onClicked: if (paramRow.page) paramRow.page.toggleFavorite(paramRow.paramKey)
    }

    Loader {
        width: paramRow.stacked ? parent.width : implicitWidth
        sourceComponent: !paramRow.meta ? null
                       : paramRow.meta.kind === "spin"         ? spinComponent
                       : paramRow.meta.kind === "switch"       ? switchComponent
                       : paramRow.meta.kind === "baudrate"     ? baudrateComponent
                       : paramRow.meta.kind === "settingsFile" ? settingsFileComponent
                                                               : tabsComponent
    }

    Component {
        id: spinComponent
        UsblSpin {
            from: paramRow.meta.from
            to: paramRow.meta.to
            stepSize: paramRow.meta.step
            fontPixelSize: Tokens.fontLg
            devValue: paramRow.paramValue
            writeBack: function(v) { paramRow.writeParam(v) }
        }
    }

    Component {
        id: switchComponent
        KSwitch {
            flat: true
            readonly property bool wantChecked: !!paramRow.paramValue
            property bool _g: false
            onWantCheckedChanged: { if (checked !== wantChecked) { _g = true; checked = wantChecked; _g = false } }
            Component.onCompleted: { _g = true; checked = wantChecked; _g = false }
            onToggled: { if (!_g) paramRow.writeParam(checked) }
        }
    }

    Component {
        id: baudrateComponent
        Row {
            width: parent ? parent.width : implicitWidth
            spacing: Tokens.spaceMd
            readonly property real setW: Math.round(120 * AppPalette.scale)
            KCombo {
                width: parent.width - parent.setW - Tokens.spaceMd
                height: Tokens.controlHMd
                model: paramRow.page ? paramRow.page.baudrateOptions : []
                currentIndex: paramRow.page ? paramRow.page.baudrateIndex : 0
                onActivated: function(index) { if (paramRow.page) paramRow.page.baudrateIndex = index }
            }
            UsblButton {
                width: parent.setW; height: Tokens.controlHMd; fontPixelSize: Tokens.fontMd
                text: qsTr("Set baudrate")
                toolTipText: qsTr("Apply the selected baud rate")
                onClicked: if (paramRow.page) paramRow.page.applyBaudrate()
            }
        }
    }

    Component {
        id: settingsFileComponent
        Row {
            width: parent ? parent.width : implicitWidth
            spacing: Tokens.spaceMd
            readonly property real bw: (width - Tokens.spaceMd) / 2
            UsblButton {
                width: parent.bw; height: Tokens.controlHMd; fontPixelSize: Tokens.fontMd
                text: qsTr("Import")
                toolTipText: qsTr("Load all device settings from an XML file")
                onClicked: if (paramRow.page) paramRow.page.openImportSettingsFile()
            }
            UsblButton {
                width: parent.bw; height: Tokens.controlHMd; fontPixelSize: Tokens.fontMd
                text: qsTr("Export")
                toolTipText: qsTr("Save all device settings to an XML file")
                onClicked: if (paramRow.page) paramRow.page.openExportSettingsFile()
            }
        }
    }

    Component {
        id: tabsComponent
        KTabBar {
            width: parent ? parent.width : implicitWidth
            trackColor: AppPalette.bgDeep
            options: paramRow.meta.options
            readonly property int wantValue: paramRow.paramValue
            property bool _g: false
            onWantValueChanged: { if (currentValue !== wantValue) { _g = true; currentValue = wantValue; _g = false } }
            Component.onCompleted: { _g = true; currentValue = wantValue; _g = false }
            onValueSelected: function(v) { if (!_g) paramRow.writeParam(v) }
        }
    }
}
