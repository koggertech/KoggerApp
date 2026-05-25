import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import QtCore
import kqml_types 1.0
import app 1.0
import "../controls"
import "../menus"
import "../scene2d"


MenuFrame {
    id: mosaicViewSettings

    signal mosaicLAngleOffsetChanged(int val)
    signal mosaicRAngleOffsetChanged(int val)

    readonly property Item _overlayItem: Window.window ? Window.window.contentItem : null
    parent: _overlayItem
    z: 1000

    function _updatePosition() {
        if (!parent || !mosaicViewCheckButton) return
        var p = mosaicViewCheckButton.mapToItem(parent, 0, 0)
        var nx = p.x + mosaicViewCheckButton.width / 2 - width / 2
        var ny = p.y - height
        if (nx < 0) nx = 0
        if (ny < 0) ny = 0
        if (parent.width  > 0 && nx + width  > parent.width)  nx = parent.width  - width
        if (parent.height > 0 && ny + height > parent.height) ny = parent.height - height
        x = nx
        y = ny
    }
    onWidthChanged:  _updatePosition()
    onHeightChanged: _updatePosition()
    Connections {
        target: mosaicViewSettings.mosaicViewCheckButton
        ignoreUnknownSignals: true
        function onXChanged()      { mosaicViewSettings._updatePosition() }
        function onYChanged()      { mosaicViewSettings._updatePosition() }
        function onWidthChanged()  { mosaicViewSettings._updatePosition() }
        function onHeightChanged() { mosaicViewSettings._updatePosition() }
    }

    property CheckButton mosaicViewCheckButton

    function updateMosaic() {
        updateMosaicButton.clicked();
    }

    function setChannelNamesToBackend() {
        core.setMosaicChannels(channel1Combo.currentText, channel2Combo.currentText);
    }

    function prevTheme() {
        mosaicTheme.currentIndex = Math.max(0, mosaicTheme.currentIndex - 1)
    }

    function nextTheme() {
        mosaicTheme.currentIndex = Math.min(mosaicTheme.model.length - 1, mosaicTheme.currentIndex + 1)
    }

    function syncLevelsSlider() {
        mosaicLevelsSlider.startPointY = mosaicLevelsSlider.valueToPosition(mosaicLevelsSlider.startValue)
        mosaicLevelsSlider.stopPointY = mosaicLevelsSlider.valueToPosition(mosaicLevelsSlider.stopValue)
        mosaicLevelsSlider.update()
    }

    function lowLevelUp(step) {
        const delta = step === undefined ? 1 : step
        const nextLow = Math.min(mosaicLevelsSlider.to, mosaicLevelsSlider.startValue + delta)
        mosaicLevelsSlider.startValue = nextLow
        if (mosaicLevelsSlider.startValue > mosaicLevelsSlider.stopValue) {
            mosaicLevelsSlider.stopValue = mosaicLevelsSlider.startValue
        }
        syncLevelsSlider()
    }

    function lowLevelDown(step) {
        const delta = step === undefined ? 1 : step
        mosaicLevelsSlider.startValue = Math.max(mosaicLevelsSlider.from, mosaicLevelsSlider.startValue - delta)
        syncLevelsSlider()
    }

    function highLevelUp(step) {
        const delta = step === undefined ? 1 : step
        mosaicLevelsSlider.stopValue = Math.min(mosaicLevelsSlider.to, mosaicLevelsSlider.stopValue + delta)
        syncLevelsSlider()
    }

    function highLevelDown(step) {
        const delta = step === undefined ? 1 : step
        const nextHigh = Math.max(mosaicLevelsSlider.from, mosaicLevelsSlider.stopValue - delta)
        mosaicLevelsSlider.stopValue = nextHigh
        if (mosaicLevelsSlider.stopValue < mosaicLevelsSlider.startValue) {
            mosaicLevelsSlider.startValue = mosaicLevelsSlider.stopValue
        }
        syncLevelsSlider()
    }

    readonly property bool anyComboPopupOpen:
        (mosaicTheme.popup    && mosaicTheme.popup.visible)    ||
        (channel1Combo.popup  && channel1Combo.popup.visible)  ||
        (channel2Combo.popup  && channel2Combo.popup.visible)  ||
        (mosaicSource.popup   && mosaicSource.popup.visible)

    readonly property bool anyHoverSource:
        mosaicViewCheckButton.hovered     ||
        isHovered                         ||
        anyComboPopupOpen                 ||
        mosaicTheme.activeFocus           ||
        channel1Combo.activeFocus         ||
        channel2Combo.activeFocus         ||
        mosaicSource.activeFocus          ||
        mosaicLAngleOffset.activeFocus    ||
        mosaicRAngleOffset.activeFocus    ||
        fakeCoordsLastNSlider.activeFocus ||
        fakeCoordsClearOldDataCheck.activeFocus

    visible: Qt.platform.os === "android"
             ? (mosaicViewCheckButton.mosaicLongPressTriggered || anyHoverSource)
             : anyHoverSource

    backgroundColor: AppPalette.bg
    horizontalMargins: Tokens.spaceLg
    verticalMargins: Tokens.spaceLg
    spacing: Tokens.spaceMd

    onIsHoveredChanged: {
        if (Qt.platform.os === "android") {
            if (isHovered) {
                isHovered = false
            }
        }
    }

    onVisibleChanged: {
        if (visible) {
            focus = true;
            _updatePosition()
        }
    }

    onFocusChanged: {
        if (Qt.platform.os === "android" && !focus) {
            Qt.callLater(function() {
                if (!mosaicViewSettings.focus) {
                    mosaicViewCheckButton.mosaicLongPressTriggered = false
                }
            })
        }
    }

    ColumnLayout {
        spacing: Tokens.spaceMd

        RowLayout {
            Layout.fillWidth: true
            spacing: Tokens.spaceMd

            Rectangle { Layout.fillWidth: true; height: 1; color: AppPalette.border }
            Text {
                text: qsTr("Mosaic settings")
                color: AppPalette.textSecond
                font.pixelSize: Tokens.fontMd
                font.bold: true
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: AppPalette.border }
        }

        RowLayout {
            spacing: Tokens.spaceMd

            ColumnLayout {
                Text {
                    Layout.fillWidth: true
                    Layout.topMargin: 0
                    Layout.preferredWidth: Tokens.controlHMd * 1.2
                    horizontalAlignment: Text.AlignHCenter
                    text: mosaicLevelsSlider.stopValue
                    color: AppPalette.text
                    font.pixelSize: Tokens.fontSm
                }
                ChartLevel {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.preferredWidth: Tokens.controlHMd * 1.2
                    id: mosaicLevelsSlider
                    Layout.alignment: Qt.AlignHCenter

                    onStartValueChanged: {
                       MosaicViewControlMenuController.onLevelChanged(startValue, stopValue);
                    }

                    onStopValueChanged: {
                       MosaicViewControlMenuController.onLevelChanged(startValue, stopValue);
                    }

                    Component.onCompleted: {
                        MosaicViewControlMenuController.onLevelChanged(startValue, stopValue);
                    }

                    Settings {
                        property alias mosaicLevelsStart: mosaicLevelsSlider.startValue
                        property alias mosaicLevelsStop: mosaicLevelsSlider.stopValue
                    }
                }
                Text {
                    Layout.fillWidth: true
                    Layout.preferredWidth: Tokens.controlHMd * 1.2
                    Layout.bottomMargin: 0
                    horizontalAlignment: Text.AlignHCenter
                    text: mosaicLevelsSlider.startValue
                    color: AppPalette.text
                    font.pixelSize: Tokens.fontSm
                }
            }

            ColumnLayout {
                id: mosaicSettingsRightColumn
                spacing: Tokens.spaceMd

                readonly property int labelW: Math.round(160 * AppPalette.scale)
                readonly property int ctrlW:  Math.round(240 * AppPalette.scale)

                RowLayout {
                    spacing: Tokens.spaceMd
                    Text {
                        text: qsTr("Theme:")
                        color: AppPalette.textSecond
                        font.pixelSize: Tokens.fontMd
                        Layout.preferredWidth: mosaicSettingsRightColumn.labelW
                    }
                    KCombo {
                        id: mosaicTheme
                        Layout.preferredWidth: mosaicSettingsRightColumn.ctrlW

                        model: [qsTr("Blue"), qsTr("Sepia"), qsTr("Sepia New"), qsTr("WRGBD"), qsTr("WhiteBlack"), qsTr("BlackWhite"), qsTr("DeepBlue"), qsTr("Ice"), qsTr("Green"), qsTr("Midnight")]
                        currentIndex: 0
                        onCurrentIndexChanged: {
                            MosaicViewControlMenuController.onThemeChanged(currentIndex)
                        }

                        onActiveFocusChanged: {
                            if (Qt.platform.os === 'android') {
                                mosaicViewSettings.focus = true
                            }
                        }

                        Component.onCompleted: {
                            MosaicViewControlMenuController.onThemeChanged(currentIndex)
                        }

                        Settings {
                            property alias mosaicTheme: mosaicTheme.currentIndex
                        }
                    }
                }

                RowLayout {
                    spacing: Tokens.spaceMd
                    Text {
                        text: qsTr("Channels:")
                        color: AppPalette.textSecond
                        font.pixelSize: Tokens.fontMd
                        Layout.preferredWidth: mosaicSettingsRightColumn.labelW
                        Layout.alignment: Qt.AlignTop
                    }
                    ColumnLayout {
                        id: rowDataset
                        spacing: Tokens.spaceXs

                        Layout.preferredWidth: mosaicSettingsRightColumn.ctrlW

                        KCombo  {
                            id: channel1Combo
                            Layout.preferredWidth: mosaicSettingsRightColumn.ctrlW

                            property bool suppressTextSignal: false
                            visible: true

                            onActiveFocusChanged: {
                                if (Qt.platform.os === 'android') {
                                    mosaicViewSettings.focus = true
                                }
                            }

                            onCurrentTextChanged: {
                                if (suppressTextSignal) {
                                    return
                                }

                                mosaicViewSettings.setChannelNamesToBackend()
                            }

                            Component.onCompleted: {
                                model = dataset.channelsNameList()

                                let index = model.indexOf(core.ch1Name)
                                if (index >= 0) {
                                    channel1Combo.currentIndex = index
                                }
                            }

                            Connections {
                                target: core
                                function onChannelListUpdated() {
                                    let list = dataset.channelsNameList()

                                    channel1Combo.suppressTextSignal = true

                                    channel1Combo.model = []
                                    channel1Combo.model = list

                                    let newIndex = list.indexOf(core.ch1Name)
                                    if (newIndex >= 0) {
                                        channel1Combo.currentIndex = newIndex
                                    }
                                    else {
                                        channel1Combo.currentIndex = 0
                                    }

                                    mosaicViewSettings.setChannelNamesToBackend()

                                    channel1Combo.suppressTextSignal = false
                                }
                            }
                        }

                        KCombo  {
                            id: channel2Combo

                            property bool suppressTextSignal: false

                            Layout.preferredWidth: mosaicSettingsRightColumn.ctrlW
                            visible: true

                            onActiveFocusChanged: {
                                if (Qt.platform.os === 'android') {
                                    mosaicViewSettings.focus = true
                                }
                            }

                            onCurrentTextChanged: {
                                if (suppressTextSignal) {
                                    return
                                }

                                mosaicViewSettings.setChannelNamesToBackend()
                            }

                            Component.onCompleted: {
                                model = dataset.channelsNameList()

                                let index = model.indexOf(core.ch2Name)
                                if (index >= 0) {
                                    channel2Combo.currentIndex = index
                                }
                            }

                            Connections {
                                target: core
                                function onChannelListUpdated() {
                                    let list = dataset.channelsNameList()

                                    channel2Combo.suppressTextSignal = true

                                    channel2Combo.model = []
                                    channel2Combo.model = list

                                    let newIndex = list.indexOf(core.ch2Name)

                                    if (newIndex >= 0) {
                                        channel2Combo.currentIndex = newIndex
                                    }
                                    else {
                                        channel2Combo.currentIndex = 0
                                    }

                                    mosaicViewSettings.setChannelNamesToBackend()

                                    channel2Combo.suppressTextSignal = false
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    spacing: Tokens.spaceMd
                    Text {
                        text: qsTr("Angle, °:")
                        color: AppPalette.textSecond
                        font.pixelSize: Tokens.fontMd
                        Layout.preferredWidth: mosaicSettingsRightColumn.labelW
                        Layout.alignment: Qt.AlignTop
                    }
                    ColumnLayout {
                        spacing: Tokens.spaceXs
                        Layout.preferredWidth: mosaicSettingsRightColumn.ctrlW
                        KSpinBox  {
                            id: mosaicLAngleOffset
                            Layout.preferredWidth: mosaicSettingsRightColumn.ctrlW
                            from: -90
                            to: 90
                            stepSize: 1
                            value: 0

                            onValueModified: function(v) {
                                mosaicViewSettings.mosaicLAngleOffsetChanged(v)
                                MosaicViewControlMenuController.onSetLAngleOffset(v)
                                dataset.onSetLAngleOffset(v);
                            }

                            Component.onCompleted: {
                                mosaicViewSettings.mosaicLAngleOffsetChanged(value)
                                MosaicViewControlMenuController.onSetLAngleOffset(value)
                                dataset.onSetLAngleOffset(value);
                            }

                            Settings {
                                property alias mosaicLAngleOffset: mosaicLAngleOffset.value
                            }
                        }

                        KSpinBox  {
                            id: mosaicRAngleOffset
                            Layout.preferredWidth: mosaicSettingsRightColumn.ctrlW
                            from: -90
                            to: 90
                            stepSize: 1
                            value: 0

                            onValueModified: function(v) {
                                mosaicViewSettings.mosaicRAngleOffsetChanged(v)
                                MosaicViewControlMenuController.onSetRAngleOffset(v)
                                dataset.onSetRAngleOffset(v);
                            }

                            Component.onCompleted: {
                                mosaicViewSettings.mosaicRAngleOffsetChanged(value)
                                MosaicViewControlMenuController.onSetRAngleOffset(value)
                                dataset.onSetRAngleOffset(value);
                            }

                            Settings {
                                property alias mosaicRAngleOffset: mosaicRAngleOffset.value
                            }
                        }
                    }
                }

                KSwitch {
                    id: mosaicTraceLine
                    text: qsTr("Trace line")
                    checked: true
                    Layout.fillWidth: true

                    onToggled: {
                        MosaicViewControlMenuController.onMeasLineVisibleChanged(checked)
                    }

                    onActiveFocusChanged: {
                        mosaicViewSettings.focus = true
                    }

                    Component.onCompleted: {
                        MosaicViewControlMenuController.onMeasLineVisibleChanged(checked)
                    }

                    Settings {
                        property alias mosaicTraceLine: mosaicTraceLine.checked
                    }
                }

                RowLayout {
                    spacing: Tokens.spaceMd
                    Text {
                        text: qsTr("Source:")
                        color: AppPalette.textSecond
                        font.pixelSize: Tokens.fontMd
                        Layout.preferredWidth: mosaicSettingsRightColumn.labelW
                    }
                    KCombo {
                        id: mosaicSource
                        Layout.preferredWidth: mosaicSettingsRightColumn.ctrlW

                        model: [qsTr("Raw"), qsTr("Side-Scan"), qsTr("TGC")]
                        currentIndex: 1  // дефолт — Side-Scan

                        onCurrentIndexChanged: {
                            core.setMosaicSource(currentIndex)
                        }

                        onActiveFocusChanged: {
                            if (Qt.platform.os === 'android') {
                                mosaicViewSettings.focus = true
                            }
                        }

                        Component.onCompleted: {
                            core.setMosaicSource(currentIndex)
                        }

                        Settings {
                            property alias mosaicSource: mosaicSource.currentIndex
                        }
                    }
                }

                // Grouped inside a bordered frame with the route_crossed_out icon
                // on top (tinted via Button.icon.color so the SVG colors are
                // overridden by Qt's icon coloring system).
                Rectangle {
                    id: fakeCoordsGroup
                    visible: core.posZeroing
                    Layout.fillWidth: true
                    Layout.topMargin: Tokens.spaceMd
                    implicitHeight: fakeCoordsGroupContent.implicitHeight + 2 * Tokens.spaceLg
                    color: "transparent"
                    border.color: AppPalette.border
                    border.width: 1
                    radius: Tokens.radiusMd

                    ColumnLayout {
                        id: fakeCoordsGroupContent
                        anchors.fill: parent
                        anchors.margins: Tokens.spaceLg
                        spacing: Tokens.spaceMd

                        Button {
                            Layout.alignment: Qt.AlignHCenter
                            flat: true
                            enabled: false
                            padding: 0
                            background: null
                            icon.source: "qrc:/icons/ui/route_crossed_out.svg"
                            icon.color: AppPalette.text
                            icon.width: Tokens.controlHMd * 1.1
                            icon.height: Tokens.controlHMd * 1.1
                            implicitWidth: Tokens.controlHMd * 1.1
                            implicitHeight: Tokens.controlHMd * 1.1
                        }

                        RowLayout {
                            spacing: Tokens.spaceMd
                            Text {
                                text: qsTr("Calc last N epochs:")
                                color: AppPalette.textSecond
                                font.pixelSize: Tokens.fontMd
                                Layout.preferredWidth: mosaicSettingsRightColumn.labelW
                            }
                            KSlider {
                                id: fakeCoordsLastNSlider
                                Layout.preferredWidth: mosaicSettingsRightColumn.ctrlW - Math.round(90 * AppPalette.scale)
                                from: 10
                                to: 3000
                                stepSize: 10
                                value: 500

                                // Gate the effective value on core.posZeroing — the cap only applies in
                                // FAKE_COORDS. Slider at "to" means "All" (no cap, process every epoch).
                                readonly property int effectiveN: (core.posZeroing && value < to) ? value : 0

                                onEffectiveNChanged: core.setMosaicFakeCoordsLastN(effectiveN)
                                Component.onCompleted: core.setMosaicFakeCoordsLastN(effectiveN)

                                Settings {
                                    property alias fakeCoordsLastNSlider: fakeCoordsLastNSlider.value
                                }
                            }
                            Text {
                                Layout.preferredWidth: Math.round(50 * AppPalette.scale)
                                horizontalAlignment: Text.AlignRight
                                color: AppPalette.text
                                font.pixelSize: Tokens.fontMd
                                text: fakeCoordsLastNSlider.value >= fakeCoordsLastNSlider.to
                                      ? qsTr("All") : fakeCoordsLastNSlider.value
                            }
                        }

                        KSwitch {
                            id: fakeCoordsClearOldDataCheck
                            text: qsTr("Clear old data (*)")
                            checked: true
                            Layout.fillWidth: true

                            readonly property bool effectiveClearOldData: checked && core.posZeroing

                            onEffectiveClearOldDataChanged: core.setMosaicFakeCoordsClearOldData(effectiveClearOldData)
                            Component.onCompleted: core.setMosaicFakeCoordsClearOldData(effectiveClearOldData)

                            Settings {
                                property alias fakeCoordsClearOldDataCheck: fakeCoordsClearOldDataCheck.checked
                            }
                        }
                    }
                }
            }
        }
    }
}
