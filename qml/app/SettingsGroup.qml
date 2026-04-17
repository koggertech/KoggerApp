import QtQuick 2.15

Item {
    id: root

    property string title: ""
    property bool collapsible: true
    property bool collapsedByDefault: true
    property bool expanded: !collapsedByDefault
    property var stateStore: null
    property string stateKey: ""
    property real preferredWidth: 250
    property int contentSpacing: 10
    property int contentInset: 6
    property color titleColor: "#E2E8F0"
    property int titlePixelSize: 15
    default property alias contentData: contentColumn.data

    property bool _stateReady: false

    width: preferredWidth
    implicitWidth: preferredWidth
    implicitHeight: contentWrapper.implicitHeight

    function loadExpandedState() {
        if (!stateStore || typeof stateStore.isSettingsGroupExpanded !== "function") {
            _stateReady = true
            return
        }

        var key = typeof stateKey === "string" ? stateKey.trim() : ""
        if (key === "") {
            _stateReady = true
            return
        }

        expanded = stateStore.isSettingsGroupExpanded(key)
        _stateReady = true
    }

    onStateStoreChanged: {
        _stateReady = false
        loadExpandedState()
    }

    onStateKeyChanged: {
        _stateReady = false
        loadExpandedState()
    }

    onExpandedChanged: {
        if (_stateReady && stateStore && typeof stateStore.setSettingsGroupExpanded === "function") {
            var key = typeof stateKey === "string" ? stateKey.trim() : ""
            if (key !== "")
                stateStore.setSettingsGroupExpanded(key, expanded)
        }
    }

    Connections {
        target: root.stateStore
        ignoreUnknownSignals: true

        function onSettingsGroupExpandedMapChanged() {
            root.loadExpandedState()
        }
    }

    Component.onCompleted: loadExpandedState()

    Column {
        id: contentWrapper

        width: root.width
        spacing: 8

        Rectangle {
            visible: root.title !== ""
            width: parent.width
            height: 36
            radius: 8
            color: headerMouse.containsMouse ? "#0B1220" : "#1E293B"
            border.width: 1
            border.color: root.expanded ? "#64748B" : "#334155"

            Rectangle {
                width: 4
                height: parent.height - 10
                radius: 2
                anchors.left: parent.left
                anchors.leftMargin: 5
                anchors.verticalCenter: parent.verticalCenter
                color: root.expanded ? "#60A5FA" : (headerMouse.containsMouse ? "#64748B" : "#475569")
            }

            Row {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 10
                spacing: 8

                DisclosureIndicator {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 10
                    height: 10
                    expanded: root.expanded
                    indicatorColor: "#CBD5E1"
                    visible: root.collapsible
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.title
                    color: root.titleColor
                    font.pixelSize: Math.max(16, root.titlePixelSize)
                    font.bold: true
                }
            }

            MouseArea {
                id: headerMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: root.collapsible ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: {
                    if (root.collapsible)
                        root.expanded = !root.expanded
                }
            }
        }

        Column {
            id: contentColumn

            visible: !root.collapsible || root.expanded
            x: root.contentInset
            width: Math.max(0, parent.width - root.contentInset * 2)
            spacing: root.contentSpacing
        }
    }
}
