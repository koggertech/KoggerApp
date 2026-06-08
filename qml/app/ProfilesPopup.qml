import QtQuick 2.15
import QtQuick.Dialogs
import QtCore
import kqml_types 1.0

// Floating settings-profile palette — same narrow pill as the bottom-track
// editor (BasePanePopup: drag / snap / collision / position-persistence). Lists
// added profiles as buttons labelled with the filename's first chars; click
// applies the profile to ALL sonars. Add (+) picks an XML file; Edit toggles
// delete-mode (click a profile to remove).
BasePanePopup {
    id: root

    required property var store

    property bool _editMode: false

    readonly property real _s: 1.5 * (theme ? theme.resCoeff : 1.0)
    readonly property int _controlH: Math.round(36 * _s) - 2
    readonly property int _sidePad: Math.round(3 * _s)
    readonly property int _gap: Math.round(6 * _s)

    // close + N profiles + add + edit.
    readonly property int _profileCount: store && store.settingsProfiles ? store.settingsProfiles.length : 0
    readonly property int _buttonCount: _profileCount + 3
    readonly property real _pillW: _controlH + _sidePad * 2
    readonly property real _pillH: _buttonCount * _controlH + (_buttonCount - 1) * _gap + _sidePad * 2
    readonly property real _wantW: _pillW + contentPadding * 2
    readonly property real _wantH: headerHeight + _pillH + contentPadding

    popupVisible: store.profilesPopupOpen
    dragEnabled: true
    resizeEnabled: false
    collapseButtonVisible: false
    fullscreenMode: false
    panelColor: "transparent"
    panelBorderColor: "transparent"
    headerDragBarLength: Math.max(Math.round(24 * _s), _pillW - _sidePad * 2)
    siblingSnapAlignTop: true

    property var _profileFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
    Settings { property alias profilesImportFolder: root._profileFolder }

    function _applySize() {
        expandedWidth = _wantW
        expandedHeight = _wantH
    }

    function syncFromStore() {
        if (!popupVisible)
            return
        suspendSignals = true
        var p = store.profilesPopupPosition(popupWidth, popupHeight)
        panelX = clampX(p.x)
        panelY = clampY(p.y)
        suspendSignals = false
    }

    function _fileName(p) {
        var s = String(p)
        var i = Math.max(s.lastIndexOf("/"), s.lastIndexOf("\\"))
        return i >= 0 ? s.slice(i + 1) : s
    }

    // Short label for the profile button — first chars of the filename (no extension).
    function _profileLabel(p) {
        var n = _fileName(p)
        var dot = n.lastIndexOf(".")
        if (dot > 0) n = n.slice(0, dot)
        return n.slice(0, 3)
    }

    // FileDialog.selectedFile is a "file:///..." string — strip the scheme to a
    // real local path (QFile can't open the URL form).
    function _localPath(u) {
        if (!u) return ""
        if (typeof u !== "string" && u.toLocalFile) {
            var lp = u.toLocalFile()
            if (lp && lp.length) return lp
        }
        var s = String(u)
        if (s.indexOf("file:///") === 0)
            return Qt.platform.os === "windows" ? s.slice(8) : s.slice(7)
        if (s.indexOf("file://") === 0)
            return s.slice(7)
        return s
    }

    // Apply a profile XML to every connected sonar.
    function _applyProfile(path) {
        if (!path || !path.length
                || typeof deviceManagerWrapper === "undefined"
                || !deviceManagerWrapper || !deviceManagerWrapper.devs)
            return
        for (var i = 0; i < deviceManagerWrapper.devs.length; ++i) {
            var d = deviceManagerWrapper.devs[i]
            if (d && d.devType !== 0 && d.importSettingsFromXML)
                d.importSettingsFromXML(path)
        }
    }

    on_WantWChanged: _applySize()
    on_WantHChanged: _applySize()

    Component.onCompleted: {
        _applySize()
        syncFromStore()
        Qt.callLater(syncFromStore)
    }

    onPopupVisibleChanged: {
        if (popupVisible) {
            _applySize()
            syncFromStore()
            Qt.callLater(syncFromStore)
        } else {
            _editMode = false
        }
    }

    onPositionCommitted: function(x, y, w, h) {
        if (popupVisible)
            store.setProfilesPopupPosition(x, y, w, h)
    }

    onCloseRequested: store.profilesPopupOpen = false

    FileDialog {
        id: addProfileDialog
        title: qsTr("Add profile")
        fileMode: FileDialog.OpenFile
        nameFilters: ["XML files (*.xml)"]
        onCurrentFolderChanged: root._profileFolder = currentFolder
        onAccepted: {
            root._profileFolder = addProfileDialog.currentFolder
            var lp = root._localPath(addProfileDialog.selectedFile)
            if (lp.length)
                root.store.addSettingsProfile(lp)
        }
    }

    // ── Content pill (below the header drag bar) ──
    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: AppPalette.bg
        border.width: 1
        border.color: AppPalette.border

        Column {
            anchors.horizontalCenter: parent.horizontalCenter
            y: root._sidePad
            spacing: root._gap

            // Close.
            KCircleIconButton {
                width: root._controlH
                height: root._controlH
                iconSource: "qrc:/icons/ui/x.svg"
                iconTintColor: AppPalette.text
                toolTipText: qsTr("Close")
                fillColor:        AppPalette.card
                fillHoverColor:   AppPalette.cardHover
                fillPressedColor: AppPalette.bgDeep
                borderColor:      AppPalette.border
                borderHoverColor: AppPalette.borderHover
                onClicked: root.store.profilesPopupOpen = false
            }

            // Profiles — labelled by filename; click applies (or removes in edit mode).
            Repeater {
                model: root.store ? root.store.settingsProfiles : 0
                delegate: KCircleIconButton {
                    required property var modelData
                    required property int index
                    width: root._controlH
                    height: root._controlH
                    glyph: modelData && modelData.path ? root._profileLabel(modelData.path) : String(index + 1)
                    glyphPixelSize: Math.round(12 * root._s)
                    glyphColor:       root._editMode ? AppPalette.dangerText : AppPalette.text
                    toolTipText: root._editMode
                                 ? qsTr("Remove profile")
                                 : (modelData && modelData.path ? root._fileName(modelData.path) : "")
                    fillColor:        root._editMode ? AppPalette.dangerBg : AppPalette.card
                    fillHoverColor:   root._editMode ? AppPalette.dangerHover : AppPalette.cardHover
                    fillPressedColor: AppPalette.bgDeep
                    borderColor:      root._editMode ? AppPalette.dangerBorder : AppPalette.border
                    borderHoverColor: root._editMode ? AppPalette.dangerBorder : AppPalette.borderHover
                    onClicked: {
                        if (root._editMode)
                            root.store.removeSettingsProfile(index)
                        else if (modelData && modelData.path)
                            root._applyProfile(modelData.path)
                    }
                }
            }

            // Add (pick XML file).
            KCircleIconButton {
                width: root._controlH
                height: root._controlH
                iconSource: "qrc:/icons/ui/file_import.svg"
                iconTintColor: AppPalette.text
                toolTipText: qsTr("Add profile")
                fillColor:        AppPalette.card
                fillHoverColor:   AppPalette.cardHover
                fillPressedColor: AppPalette.bgDeep
                borderColor:      AppPalette.border
                borderHoverColor: AppPalette.borderHover
                onClicked: {
                    root._editMode = false   // adding isn't deleting
                    addProfileDialog.currentFolder = root._profileFolder
                    addProfileDialog.open()
                }
            }

            // Edit — toggles delete-mode for the numbered profiles.
            KCircleIconButton {
                width: root._controlH
                height: root._controlH
                iconSource: "qrc:/icons/ui/pencil.svg"
                iconTintColor: AppPalette.text
                toolTipText: qsTr("Edit profiles")
                fillColor:        root._editMode ? AppPalette.accentBgStrong : AppPalette.card
                fillHoverColor:   root._editMode ? AppPalette.accentBorder : AppPalette.cardHover
                fillPressedColor: AppPalette.bgDeep
                borderColor:      root._editMode ? AppPalette.accentBorder : AppPalette.border
                borderHoverColor: root._editMode ? AppPalette.accentBorder : AppPalette.borderHover
                onClicked: root._editMode = !root._editMode
            }
        }
    }
}
