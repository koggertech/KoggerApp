import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQml.Models

Item {
    id: root

    property var geo: null
    property var view: null
    property bool expanded: false
    property real panelWidth: theme.controlHeight * 10
    property real panelPadding: 10

    function toLocalPath(url) {
        var s = url.toString()
        if (s.indexOf("file:///") === 0) {
            return s.replace("file:///", Qt.platform.os === "windows" ? "" : "/")
        }
        if (s.indexOf("file://") === 0) {
            return s.replace("file://", "")
        }
        return s
    }

    function targetFolderId() {
        if (!geo) return ""
        if (geo.selectedNodeIsFolder && geo.selectedNodeId !== "") {
            return geo.selectedNodeId
        }
        return geo.currentFolderId
    }

    Rectangle {
        id: panel
        width: root.expanded ? root.panelWidth : 0
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        color: theme.menuBackColor
        border.color: theme.controlBorderColor
        border.width: 1
        radius: 4
        clip: true
        opacity: root.expanded ? 1.0 : 0.0

        Behavior on width { NumberAnimation { duration: 160; easing.type: Easing.OutQuad } }
        Behavior on opacity { NumberAnimation { duration: 120 } }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: root.panelPadding
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                CheckButton {
                    checkable: false
                    iconSource: "qrc:/icons/ui/file_import.svg"
                    implicitWidth: theme.controlHeight * 1.2
                    implicitHeight: theme.controlHeight * 1.2
                    onClicked: openKgtDialog.open()
                }

                CheckButton {
                    checkable: false
                    iconSource: "qrc:/icons/ui/file_export.svg"
                    implicitWidth: theme.controlHeight * 1.2
                    implicitHeight: theme.controlHeight * 1.2
                    enabled: geo !== null
                    onClicked: saveKgtDialog.open()
                }

                CheckButton {
                    checkable: false
                    iconSource: "qrc:/icons/ui/file_plus.svg"
                    implicitWidth: theme.controlHeight * 1.2
                    implicitHeight: theme.controlHeight * 1.2
                    onClicked: {
                        if (geo) geo.newDocument()
                    }
                }

                CheckButton {
                    checkable: false
                    iconSource: "qrc:/icons/ui/focus_2.svg"
                    implicitWidth: theme.controlHeight * 1.2
                    implicitHeight: theme.controlHeight * 1.2
                    enabled: view !== null
                    onClicked: {
                        if (view) view.geojsonFitInView()
                    }
                }

                Item { Layout.fillWidth: true }

                Label {
                    text: geo ? geo.currentFile : ""
                    color: theme.textColor
                    elide: Label.ElideLeft
                    Layout.fillWidth: true
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                CheckButton {
                    checkable: false
                    text: "Import"
                    enabled: geo !== null
                    onClicked: importDialog.open()
                }

                CheckButton {
                    checkable: false
                    text: "Export"
                    enabled: geo !== null
                    onClicked: exportDialog.open()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 6

                    TreeView {
                        id: tree
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: geo ? geo.treeModel : null
                        selectionModel: ItemSelectionModel { model: tree.model }

                        onCurrentRowChanged: {
                            const idx = selectionModel.currentIndex
                            if (!idx.valid) return
                            if (geo) geo.selectIndex(idx)
                        }

                        delegate: Item {
                            id: nodeItem
                            width: tree.width
                            height: theme.controlHeight
                            implicitWidth: parent.width
                            implicitHeight: theme.controlHeight

                            readonly property real indentation: theme.controlHeight + 10
                            readonly property real padding: 5

                            required property TreeView treeView
                            required property bool isTreeNode
                            required property bool expanded
                            required property bool hasChildren
                            required property int depth
                            required property int row
                            required property int column
                            required property bool current

                            Item {
                                x: padding + (isTreeNode ? depth * indentation : 0)
                                width: parent.width - x
                                height: parent.height

                                RowLayout {
                                    spacing: 2

                                    CheckButton {
                                        id: item_visible_button
                                        checkable: true
                                        checked: model.visible
                                        iconSource: checked ? "qrc:/icons/ui/eye.svg" : "qrc:/icons/ui/eye-off.svg"
                                        borderColor: "transparent"
                                        backColor: "transparent"
                                        checkedBorderColor: "transparent"
                                        checkedBackColor: "transparent"
                                        color: theme.textColor
                                        checkedColor: theme.textColor
                                        text: ""
                                        onCheckedChanged: {
                                            if (geo) geo.setNodeVisible(model.id, model.isFolder, checked)
                                        }
                                    }

                                    CheckButton {
                                        id: expandButton
                                        iconSource: model.isFolder
                                                ? (expanded ? "qrc:/icons/ui/chevron-down.svg" : "qrc:/icons/ui/chevron-right.svg")
                                                : (model.geomType === "Point"
                                                   ? "qrc:/icons/ui/point.svg"
                                                   : (model.geomType === "LineString"
                                                      ? "qrc:/icons/ui/line.svg"
                                                      : (model.geomType === "Polygon"
                                                         ? "qrc:/icons/ui/polygon.svg"
                                                         : "qrc:/icons/ui/file.svg")))
                                        checkable: false
                                        borderColor: "transparent"
                                        backColor: "transparent"
                                        checkedBorderColor: "transparent"
                                        checkedBackColor: "transparent"
                                        color: theme.textColor
                                        checkedColor: theme.textColor

                                        TapHandler {
                                            onSingleTapped: {
                                                if (model.isFolder) {
                                                    treeView.toggleExpanded(row)
                                                }
                                            }
                                        }
                                    }

                                    CText {
                                        text: model.isFolder ? model.name : model.geomType
                                        small: true
                                    }

                                    CText {
                                        text: model.isFolder ? ("(" + model.vertexCount + ")") : ""
                                        small: true
                                        color: theme.disabledTextColor
                                    }

                                    Item { Layout.fillWidth: true }

                                    CheckButton {
                                        visible: model.isFolder
                                        checkable: false
                                        iconSource: "qrc:/icons/ui/plus.svg"
                                        implicitWidth: theme.controlHeight
                                        implicitHeight: theme.controlHeight
                                        backColor: "transparent"
                                        borderColor: "transparent"
                                        checkedBorderColor: "transparent"
                                        onClicked: {
                                            if (geo) {
                                                geo.selectNode(model.id, true, model.parentId)
                                                geo.addFolderToCurrent()
                                            }
                                        }
                                    }

                                    CheckButton {
                                        checkable: false
                                        iconSource: "qrc:/icons/ui/x.svg"
                                        implicitWidth: theme.controlHeight
                                        implicitHeight: theme.controlHeight
                                        backColor: "transparent"
                                        borderColor: "transparent"
                                        checkedBorderColor: "transparent"
                                        onClicked: {
                                            if (geo) {
                                                geo.selectNode(model.id, model.isFolder, model.parentId)
                                                geo.deleteNode(model.id, model.isFolder)
                                            }
                                        }
                                    }
                                }

                                TapHandler {
                                    onSingleTapped: {
                                        let index = treeView.index(row, column)
                                        treeView.selectionModel.select(index, ItemSelectionModel.SelectCurrent)
                                    }
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Item { Layout.fillWidth: true }

                        CheckButton {
                            checkable: false
                            iconSource: "qrc:/icons/ui/plus.svg"
                            implicitWidth: theme.controlHeight * 1.1
                            implicitHeight: theme.controlHeight * 1.1
                            onClicked: if (geo) geo.addFolderToRoot()
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        visible: geo ? geo.tool !== 0 : false

                        CheckButton {
                            checkable: false
                            iconSource: "qrc:/icons/ui/file-check.svg"
                            implicitWidth: theme.controlHeight * 1.1
                            implicitHeight: theme.controlHeight * 1.1
                            enabled: geo ? geo.drawing : false
                            onClicked: if (geo) geo.finishDrawing()
                        }

                        CheckButton {
                            checkable: false
                            iconSource: "qrc:/icons/ui/repeat.svg"
                            implicitWidth: theme.controlHeight * 1.1
                            implicitHeight: theme.controlHeight * 1.1
                            enabled: geo ? geo.drawing : false
                            onClicked: if (geo) geo.undoLastVertex()
                        }

                        CheckButton {
                            checkable: false
                            iconSource: "qrc:/icons/ui/x.svg"
                            implicitWidth: theme.controlHeight * 1.1
                            implicitHeight: theme.controlHeight * 1.1
                            enabled: geo ? geo.tool !== 0 : false
                            onClicked: if (geo) geo.cancelDrawing()
                        }
                    }
                }

                ColumnLayout {
                    Layout.preferredWidth: theme.controlHeight * 1.5
                    Layout.alignment: Qt.AlignTop
                    spacing: 6

                    CheckButton {
                        checkable: true
                        checked: geo ? geo.tool === 1 : false
                        iconSource: "qrc:/icons/ui/point.svg"
                        implicitWidth: theme.controlHeight * 1.3
                        implicitHeight: theme.controlHeight * 1.3
                        onClicked: {
                            if (geo) geo.tool = (geo.tool === 1 ? 0 : 1)
                        }
                    }

                    CheckButton {
                        checkable: true
                        checked: geo ? geo.tool === 2 : false
                        iconSource: "qrc:/icons/ui/line.svg"
                        implicitWidth: theme.controlHeight * 1.3
                        implicitHeight: theme.controlHeight * 1.3
                        onClicked: {
                            if (geo) geo.tool = (geo.tool === 2 ? 0 : 2)
                        }
                    }

                    CheckButton {
                        checkable: true
                        checked: geo ? geo.tool === 3 : false
                        iconSource: "qrc:/icons/ui/polygon.svg"
                        implicitWidth: theme.controlHeight * 1.3
                        implicitHeight: theme.controlHeight * 1.3
                        onClicked: {
                            if (geo) geo.tool = (geo.tool === 3 ? 0 : 3)
                        }
                    }
                }
            }

            Label {
                visible: geo ? geo.lastError !== "" : false
                text: geo ? geo.lastError : ""
                color: "#ff6b6b"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }

    FileDialog {
        id: openKgtDialog
        title: "Open KGT"
        fileMode: FileDialog.OpenFile
        nameFilters: ["Kogger Geometry Tree (*.kgt)", "All Files (*)"]
        onAccepted: {
            const path = root.toLocalPath(openKgtDialog.selectedFile)
            if (geo) geo.loadKgt(path)
        }
    }

    FileDialog {
        id: saveKgtDialog
        title: "Save KGT"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Kogger Geometry Tree (*.kgt)", "All Files (*)"]
        onAccepted: {
            const path = root.toLocalPath(saveKgtDialog.selectedFile)
            if (geo) geo.saveKgt(path)
        }
    }

    FileDialog {
        id: importDialog
        title: "Import GeoJSON"
        fileMode: FileDialog.OpenFile
        nameFilters: ["GeoJSON (*.geojson *.json)", "All Files (*)"]
        onAccepted: {
            const path = root.toLocalPath(importDialog.selectedFile)
            const folderId = root.targetFolderId()
            if (geo && folderId !== "") geo.importGeoJsonToFolder(path, folderId)
        }
    }

    FileDialog {
        id: exportDialog
        title: "Export Folder"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Kogger Geometry Tree (*.kgt)", "GeoJSON (*.geojson *.json)", "All Files (*)"]
        onAccepted: {
            const path = root.toLocalPath(exportDialog.selectedFile)
            const folderId = root.targetFolderId()
            if (geo && folderId !== "") geo.exportFolder(path, folderId)
        }
    }
}
