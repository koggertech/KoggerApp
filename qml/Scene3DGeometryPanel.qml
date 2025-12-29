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
    property real panelWidth: theme.controlHeight * 12
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
        // clip: true
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
                    text: "Import"
                    onClicked: openKgtDialog.open()
                }

                CheckButton {
                    checkable: false
                    iconSource: "qrc:/icons/ui/file_export.svg"
                    text: "Export"
                    enabled: geo !== null
                    onClicked: saveKgtDialog.open()
                }

                CheckButton {
                    checkable: false
                    iconSource: "qrc:/icons/ui/x.svg"
                    text: "Clear All"
                    onClicked: {
                        if (geo) geo.newDocument()
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                CheckButton {
                    checkable: true
                    iconSource: "qrc:/icons/ui/focus_2.svg"
                    text: "Folow"
                    enabled: view !== null
                    onClicked: {
                        if (checked && view) view.geojsonFitInView()
                    }
                }


                CheckButton {
                    checkable: false
                    iconSource: "qrc:/icons/ui/plus.svg"
                    text: "Folder"
                    onClicked: if (geo) geo.addFolderToRoot()
                }

                CheckButton {
                    checkable: false
                    iconSource: "qrc:/icons/ui/edit.svg"
                    text: "Edit"
                    onClicked: {

                    }
                }
            }

            // RowLayout {
            //     Layout.fillWidth: true
            //     spacing: 6

            //     CheckButton {
            //         checkable: false
            //         text: "Import"
            //         enabled: geo !== null
            //         onClicked: importDialog.open()
            //     }

            //     CheckButton {
            //         checkable: false
            //         text: "Export"
            //         enabled: geo !== null
            //         onClicked: exportDialog.open()
            //     }
            // }

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
                        selectionModel: ItemSelectionModel { model: tree.model} //  model: tree.model

                        onCurrentRowChanged: {
                            const idx = selectionModel.currentIndex
                            if (!idx.valid) return
                            if (geo) geo.selectIndex(idx)
                        }

                        // delegate: Item {
                        //     required property bool current
                        //     required property bool selected
                        //     required property int row
                        //     required property int column
                        //     required property var index

                        //     width: tree.width
                        //     height: theme.controlHeight
                        //     implicitWidth: parent.width
                        //     implicitHeight: theme.controlHeight


                        //     Rectangle {
                        //         anchors.fill: parent
                        //         border.width: current ? 1 : 0
                        //         color:  current ? "red" : "white"

                        //         RowLayout {
                        //             id: rowItem
                        //             anchors.fill: parent
                        //             spacing: 2

                        //             Button {
                        //                 text: display
                        //                 implicitWidth: 20
                        //                 onPressed: {
                        //                     // tree.currentIndex = index
                        //                     // tree.currentIndex = tree.index(row, column)
                        //                     // tree.selectionModel.currentIndex = index
                        //                     // tree.selectionMode
                        //                     // tree.currentRow = row
                        //                     let index = tree.index(row, column)
                        //                     tree.selectionModel.setCurrentIndex(index, ItemSelectionModel.Current)
                        //                 }
                        //             }
                        //         }
                        //     }
                        // }

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
                            required property bool selected
                            required property bool editing

                            Item {
                                x: padding + (isTreeNode ? depth * indentation : 0)
                                width: parent.width - x
                                height: parent.height

                                // border.width: nodeItem.current ? 1 : 0
                                // color: nodeItem.current ? "red" : "white"

                                Rectangle {
                                    id: backgroundRect
                                    color: current ? "red" : "white"
                                    border.color: "transparent"
                                    border.width: 0
                                    anchors.fill: rowItem
                                    opacity: 0.5
                                    radius: 2

                                    gradient: Gradient {
                                        GradientStop { position: 0.0; color: current ?  theme.controlSolidBackColor : "transparent"; }
                                        GradientStop { position: 0.85; color: current ?  theme.controlSolidBackColor : "transparent"; }
                                        GradientStop { position: 1.0; color: current ?  "red" : "transparent" }
                                    }
                                }

                                RowLayout {
                                    id: rowItem
                                    spacing: 0

                                    CheckButton {
                                        id: expandButton
                                        iconSource: expanded ? "qrc:/icons/ui/chevron-down.svg" : "qrc:/icons/ui/chevron-right.svg"
                                        checkable: false
                                        borderColor: "transparent"
                                        backColor: "transparent"
                                        checkedBorderColor: "transparent"
                                        checkedBackColor: "transparent"
                                        color: theme.textColor
                                        checkedColor: theme.textColor
                                        visible: model.isFolder

                                        TapHandler {
                                            onSingleTapped: {
                                                if (model.isFolder) {
                                                    tree.toggleExpanded(row)
                                                    let index = treeView.index(row, column)
                                                    tree.selectionModel.setCurrentIndex(index, ItemSelectionModel.Current)
                                                }
                                            }
                                        }
                                    }

                                    CheckButton {
                                        id: item_visible_button
                                        checkable: true
                                        checked: model.visible
                                        iconSource: model.isFolder ? (checked ? "qrc:/icons/ui/folder.svg" : "qrc:/icons/ui/folder-off.svg") :
                                                    (model.geomType === "Point"
                                                     ? (checked ? "qrc:/icons/ui/point.svg" : "qrc:/icons/ui/point-off.svg")
                                                     : (model.geomType === "LineString"
                                                        ? (checked ? "qrc:/icons/ui/line.svg" : "qrc:/icons/ui/line-off.svg")
                                                        : (model.geomType === "Polygon"
                                                           ? (checked ? "qrc:/icons/ui/polygon.svg" : "qrc:/icons/ui/polygon-off.svg")
                                                           : (checked ? "qrc:/icons/ui/file.svg" : "qrc:/icons/ui/file-off.svg"))))
                                        borderColor: "transparent"
                                        backColor: "transparent"
                                        checkedBorderColor: "transparent"
                                        checkedBackColor: "transparent"
                                        color: theme.textColor
                                        checkedColor: theme.textColor
                                        onCheckedChanged: {
                                            if (geo) geo.setNodeVisible(model.id, model.isFolder, checked)
                                        }
                                    }

                                    CText {
                                        text: model.isFolder ? model.name : model.geomType
                                        small: true

                                        TapHandler {
                                            onSingleTapped: {
                                                let index = treeView.index(row, column)
                                                tree.selectionModel.setCurrentIndex(index, ItemSelectionModel.Current)
                                            }

                                            onDoubleTapped: {
                                                tree.toggleExpanded(row)
                                                let index = treeView.index(row, column)
                                                tree.selectionModel.setCurrentIndex(index, ItemSelectionModel.Current)
                                            }
                                        }
                                    }

                                    CText {
                                        text: model.isFolder ? ("(" + model.vertexCount + ")") : ""
                                        small: true
                                        color: theme.disabledTextColor



                                        TapHandler {
                                            onSingleTapped: {
                                                let index = treeView.index(row, column)
                                                tree.selectionModel.setCurrentIndex(index, ItemSelectionModel.Current)
                                            }

                                            onDoubleTapped: {
                                                tree.toggleExpanded(row)
                                                let index = treeView.index(row, column)
                                                tree.selectionModel.setCurrentIndex(index, ItemSelectionModel.Current)
                                            }
                                        }
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
                                                let index = treeView.index(row, column)
                                                tree.selectionModel.setCurrentIndex(index, ItemSelectionModel.Current)
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

                Item {
                    Layout.preferredWidth: 1
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

    // FileDialog {
    //     id: importDialog
    //     title: "Import GeoJSON"
    //     fileMode: FileDialog.OpenFile
    //     nameFilters: ["GeoJSON (*.geojson *.json)", "All Files (*)"]
    //     onAccepted: {
    //         const path = root.toLocalPath(importDialog.selectedFile)
    //         const folderId = root.targetFolderId()
    //         if (geo && folderId !== "") geo.importGeoJsonToFolder(path, folderId)
    //     }
    // }

    // FileDialog {
    //     id: exportDialog
    //     title: "Export Folder"
    //     fileMode: FileDialog.SaveFile
    //     nameFilters: ["Kogger Geometry Tree (*.kgt)", "GeoJSON (*.geojson *.json)", "All Files (*)"]
    //     onAccepted: {
    //         const path = root.toLocalPath(exportDialog.selectedFile)
    //         const folderId = root.targetFolderId()
    //         if (geo && folderId !== "") geo.exportFolder(path, folderId)
    //     }
    // }
}
