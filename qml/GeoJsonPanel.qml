import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQml.Models

MenuFrame {
    id: root
    property var geo: null
    property var view: null


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

    ColumnLayout {
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            CheckButton {
                checkable: false
                iconSource: "qrc:/icons/ui/file_import.svg"
                implicitWidth: theme.controlHeight * 1.3
                implicitHeight: theme.controlHeight * 1.3
                onClicked: openKgtDialog.open()
            }

            CheckButton {
                checkable: false
                iconSource: "qrc:/icons/ui/file_export.svg"
                implicitWidth: theme.controlHeight * 1.3
                implicitHeight: theme.controlHeight * 1.3
                enabled: geo !== null
                onClicked: saveKgtDialog.open()
            }

            CheckButton {
                checkable: false
                iconSource: "qrc:/icons/ui/file_plus.svg"
                implicitWidth: theme.controlHeight * 1.3
                implicitHeight: theme.controlHeight * 1.3
                onClicked: {
                    if (geo) geo.newDocument()
                }
            }

            CheckButton {
                checkable: false
                iconSource: "qrc:/icons/ui/focus_2.svg"
                implicitWidth: theme.controlHeight * 1.3
                implicitHeight: theme.controlHeight * 1.3
                enabled: view !== null
                onClicked: {
                    if (view) view.geojsonFitInView()
                }
            }

            // Item { Layout.fillWidth: true }

            Label {
                text: geo ? geo.currentFile : ""
                color: "white"
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

            CheckButton {
                checkable: false
                text: "Add Root"
                enabled: geo !== null
                onClicked: if (geo) geo.addFolderToRoot()
            }

            CheckButton {
                checkable: false
                text: "Add Child"
                enabled: geo !== null
                onClicked: if (geo) geo.addFolderToCurrent()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            CheckButton {
                id: toolSelect
                text: "Select"
                checked: geo ? geo.tool === 0 : true
                onClicked: if (geo) geo.tool = 0
            }
            CheckButton {
                id: toolPoint
                text: "Point"
                checked: geo ? geo.tool === 1 : false
                onClicked: if (geo) geo.tool = 1
            }
            CheckButton {
                id: toolLine
                text: "Line"
                checked: geo ? geo.tool === 2 : false
                onClicked: if (geo) geo.tool = 2
            }
            CheckButton {
                id: toolPoly
                text: "Polygon"
                checked: geo ? geo.tool === 3 : false
                onClicked: if (geo) geo.tool = 3
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            CheckButton {
                checkable: false
                text: "Finish"
                enabled: geo ? geo.drawing : false
                onClicked: if (geo) geo.finishDrawing()
            }
            CheckButton {
                checkable: false
                text: "Undo"
                enabled: geo ? geo.drawing : false
                onClicked: if (geo) geo.undoLastVertex()
            }
            CheckButton {
                checkable: false
                text: "Cancel"
                enabled: geo ? geo.drawing : false
                onClicked: if (geo) geo.cancelDrawing()
            }

            CheckButton {
                checkable: false
                iconSource: "qrc:/icons/ui/x.svg"
                implicitWidth: theme.controlHeight * 1.2
                implicitHeight: theme.controlHeight * 1.2
                enabled: geo ? geo.selectedFeatureId !== "" : false
                onClicked: if (geo) geo.deleteSelectedFeature()
            }
        }



        TreeView {
            id: tree
            Layout.fillWidth: true
            Layout.preferredHeight: 10 * (theme.controlHeight + 4)
            Layout.maximumHeight: 10 * (theme.controlHeight + 4)
            implicitWidth: 100
            width: 100
            clip: true
            model: geo.treeModel

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

                readonly property real indentation: theme.controlHeight+10
                readonly property real padding: 5

                // Assigned to by TreeView:
                required property TreeView treeView
                required property bool isTreeNode
                required property bool expanded
                required property bool hasChildren
                required property int depth
                required property int row
                required property int column
                required property bool current



                Item {
                    x: padding + (isTreeNode ? (depth ) * indentation : 0)
                    width: parent.width - x
                    height: parent.height



                    RowLayout {
                    //     CheckButton {
                    //         id: indicator
                    //         x: padding + (depth * indentation)
                    //         // anchors.verticalCenter: parent.verticalCenter
                    //         visible: isTreeNode && hasChildren
                    //         iconSource: expanded ? "qrc:/icons/ui/chevron-down.svg"
                    //                                 : "qrc:/icons/ui/chevron-right.svg"
                    //         color: nodeItem.expanded ? "white" : "red"
                    //         checkable: false

                    //         TapHandler {
                    //             onSingleTapped: {
                    //                 let index = treeView.index(row, column)
                    //                 treeView.selectionModel.setCurrentIndex(index, ItemSelectionModel.NoUpdate)
                    //                 treeView.toggleExpanded(row)
                    //             }
                    //         }
                    //     }

                        Image {
                            id: name
                            source: model.isFolder
                                    ? (hasChildren ? "qrc:/icons/ui/folder_full.svg" : "qrc:/icons/ui/folder.svg")
                                    : (model.geomType === "Point"
                                       ? "qrc:/icons/ui/point.svg"
                                       : (model.geomType === "LineString"
                                          ? "qrc:/icons/ui/line.svg"
                                          : (model.geomType === "Polygon"
                                             ? "qrc:/icons/ui/polygon.svg"
                                             : "qrc:/icons/ui/file.svg")))

                            TapHandler {
                                onSingleTapped: {
                                    let index = treeView.index(row, column)
                                    treeView.selectionModel.setCurrentIndex(index, ItemSelectionModel.NoUpdate)
                                    treeView.toggleExpanded(row)
                                    // if (geo) geo.selectNode(model.id, model.isFolder, model.parentId)
                                }
                            }
                        }

                        Label {
                            id: label
                            Layout.fillWidth: true
                            width: parent.width - padding - x
                            clip: true
                            text: model.name
                            color: nodeItem.expanded && hasChildren ? "white" : "red"
                        }
                    }

                    // MouseArea {
                    //     anchors.fill: parent
                    //     propagateComposedEvents: true
                    //     onClicked: {
                    //         // let index = treeView.index(row, column)
                    //         // treeView.selectionModel.select(index, ItemSelectionModel.SelectCurrent)
                    //         // if (geo) geo.selectNode(model.id, model.isFolder, model.parentId)
                    //     }
                    // }
                }

            }
        }

        ColumnLayout {
            // anchors.fill: parent
            anchors.margins: 6
            spacing: 4

            Label {
                text: "Vertices"
                color: "white"
            }

            Repeater {
                model: geo ? geo.featureModel : null
                delegate: ColumnLayout {
                    width: parent.width
                    property string featureId: model.id
                    property string featureGeomType: model.geomType
                    property int featureVertexCount: model.vertexCount
                    property var featureCoords: model.coords ? model.coords : []
                    visible: geo && geo.selectedFeatureId === featureId
                    spacing: 2

                    Label {
                        text: featureGeomType + " (" + featureVertexCount + ")"
                        color: "#cfcfcf"
                    }

                    Repeater {
                        model: featureCoords
                        delegate: Label {
                            property var c: modelData
                            property var firstCoord: (featureCoords && featureCoords.length > 0) ? featureCoords[0] : null
                            property bool isClosing: (featureGeomType === "Polygon"
                                                      && (index === (featureCoords.length - 1))
                                                      && firstCoord
                                                      && c
                                                      && (c.lat === firstCoord.lat)
                                                      && (c.lon === firstCoord.lon)
                                                      && (c.z === firstCoord.z))

                            visible: !isClosing
                            text: "  Point #" + (index + 1)
                                  + "  Lat: " + Number(c.lat).toFixed(6)
                                  + ", Lon: " + Number(c.lon).toFixed(6)
                            color: "#e6e6e6"
                            elide: Label.ElideRight
                            Layout.fillWidth: true
                        }
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
