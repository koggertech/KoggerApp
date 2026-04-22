import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root

    property var options: []
    property var currentValue
    property int buttonHeight: 30
    property int buttonSpacing: 6
    property int horizontalPadding: 4
    property int verticalPadding: 4
    property int cornerRadius: 8
    property bool dragSelectEnabled: true
    signal valueSelected(var value)

    readonly property int optionCount: Array.isArray(options) ? options.length : 0
    readonly property real innerWidth: Math.max(0, width - horizontalPadding * 2)
    readonly property real segmentWidth: optionCount > 0
                                       ? Math.max(0, (innerWidth - (optionCount - 1) * buttonSpacing) / optionCount)
                                       : 0
    readonly property int selectedIndex: indexOfCurrentValue()
    property int dragPreviewIndex: -1
    readonly property int visualIndex: dragPreviewIndex >= 0 ? dragPreviewIndex : selectedIndex
    property bool syncingFromValue: false

    implicitWidth: Math.max(120, optionCount * 92 + Math.max(0, optionCount - 1) * buttonSpacing + horizontalPadding * 2)
    implicitHeight: buttonHeight + verticalPadding * 2

    function optionAt(idx) {
        if (!Array.isArray(options) || idx < 0 || idx >= options.length)
            return null
        return options[idx]
    }

    function optionLabelAt(idx) {
        var option = optionAt(idx)
        if (option && option.label !== undefined)
            return String(option.label)
        return String(option)
    }

    function optionValueAt(idx) {
        var option = optionAt(idx)
        if (option && option.value !== undefined)
            return option.value
        return option
    }

    function indexOfCurrentValue() {
        if (!Array.isArray(options))
            return -1
        for (var i = 0; i < options.length; ++i) {
            if (optionValueAt(i) === currentValue)
                return i
        }
        return -1
    }

    function indexFromPosition(xPos) {
        if (optionCount <= 0)
            return -1

        var localX = xPos - horizontalPadding
        if (localX < 0)
            localX = 0
        if (localX > innerWidth)
            localX = innerWidth

        var bestIndex = 0
        var bestDistance = Number.MAX_VALUE
        for (var i = 0; i < optionCount; ++i) {
            var centerX = i * (segmentWidth + buttonSpacing) + segmentWidth / 2
            var distance = Math.abs(localX - centerX)
            if (distance < bestDistance) {
                bestDistance = distance
                bestIndex = i
            }
        }
        return bestIndex
    }

    function applyIndex(indexToApply) {
        if (indexToApply < 0 || indexToApply >= optionCount)
            return

        if (tabBar.currentIndex !== indexToApply) {
            syncingFromValue = true
            tabBar.currentIndex = indexToApply
            syncingFromValue = false
        }

        var nextValue = optionValueAt(indexToApply)
        if (nextValue !== currentValue)
            valueSelected(nextValue)
    }

    function syncFromCurrentValue() {
        var idx = selectedIndex
        if (idx < 0)
            idx = 0
        if (tabBar.currentIndex === idx)
            return
        syncingFromValue = true
        tabBar.currentIndex = idx
        syncingFromValue = false
    }

    onCurrentValueChanged: {
        syncFromCurrentValue()
        if (root.dragPreviewIndex >= 0 && root.dragPreviewIndex === root.selectedIndex)
            root.dragPreviewIndex = -1
    }
    onOptionsChanged: syncFromCurrentValue()
    Component.onCompleted: syncFromCurrentValue()

    Rectangle {
        anchors.fill: parent
        radius: root.cornerRadius
        color: AppPalette.bg
        border.width: 1
        border.color: AppPalette.border
    }

    Rectangle {
        visible: root.visualIndex >= 0 && root.optionCount > 0
        x: root.horizontalPadding + root.visualIndex * (root.segmentWidth + root.buttonSpacing)
        y: root.verticalPadding
        width: root.segmentWidth
        height: root.buttonHeight
        radius: 6
        color: AppPalette.accentBg
        border.width: 1
        border.color: AppPalette.accentBorder
        z: 1

        Behavior on x {
            NumberAnimation {
                duration: 160
                easing.type: Easing.OutCubic
            }
        }

        Behavior on width {
            NumberAnimation {
                duration: 160
                easing.type: Easing.OutCubic
            }
        }
    }

    TabBar {
        id: tabBar

        x: root.horizontalPadding
        y: root.verticalPadding
        width: root.innerWidth
        height: root.buttonHeight
        spacing: root.buttonSpacing
        z: 2

        background: Item {}

        onCurrentIndexChanged: {
            if (root.syncingFromValue)
                return
            if (!dragHandler.active)
                root.applyIndex(currentIndex)
        }

        Repeater {
            model: root.optionCount

            delegate: TabButton {
                id: tabButton
                property int optionIndex: index
                readonly property bool selected: optionIndex === root.visualIndex

                width: root.segmentWidth
                height: root.buttonHeight
                text: root.optionLabelAt(optionIndex)
                padding: 0
                topPadding: 0
                bottomPadding: 0
                leftPadding: 0
                rightPadding: 0
                focusPolicy: Qt.NoFocus

                background: Item {}

                contentItem: Item {
                    implicitWidth: label.implicitWidth
                    implicitHeight: tabButton.height

                    Text {
                        id: label
                        anchors.centerIn: parent
                        text: tabButton.text
                        color: selected ? AppPalette.text : AppPalette.textSecond
                        font.pixelSize: 13
                        font.bold: true
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }

    DragHandler {
        id: dragHandler

        target: null
        enabled: root.dragSelectEnabled
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchScreen | PointerDevice.TouchPad

        onActiveChanged: {
            if (active) {
                root.dragPreviewIndex = root.indexFromPosition(centroid.position.x)
                return
            }

            var indexToApply = root.dragPreviewIndex
            root.dragPreviewIndex = -1
            root.applyIndex(indexToApply)
        }

        onCentroidChanged: {
            if (!active)
                return
            root.dragPreviewIndex = root.indexFromPosition(centroid.position.x)
        }
    }

}
