import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

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

            // Item { Layout.fillWidth: true }

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
            clip: true
            model: geo ? geo.treeModel : null


            selectionModel: ItemSelectionModel {}
            // headerVisible: false

            // TableViewColumn {
            //     role: "display"
            //     width: tree.width
            // }

            // delegate: TreeViewDelegate {}

            delegate: Rectangle {
                id: nodeItem
                width: tree.width
                height: theme.controlHeight
                radius: 2
                color: (geo && geo.selectedNodeId === model.id) ? "#2b4f7a" : "#2a2a2a"
                border.color: "transparent"
                border.width: 1

                readonly property real indentation: 20
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

                // Rotate indicator when expanded by the user
                // (requires TreeView to have a selectionModel)
                // property Animation indicatorAnimation: NumberAnimation {
                //     target: indicator
                //     property: "rotation"
                //     from: expanded ? 0 : 90
                //     to: expanded ? 90 : 0
                //     duration: 100
                //     easing.type: Easing.OutQuart
                // }
                TableView.onPooled: indicatorAnimation.complete()
                TableView.onReused: if (current) indicatorAnimation.start()
                onExpandedChanged: indicator.rotation = expanded ? 90 : 0


                Component.onCompleted: {
                    if (model.isFolder && typeof tree.expand === "function" && typeof tree.collapse === "function") {
                        if (model.expanded) {
                            tree.expand(styleData.index)
                        } else {
                            tree.collapse(styleData.index)
                        }
                    }
                }

                RowLayout {
                    id: row
                    anchors.fill: parent
                    spacing: 6

                    CButton {
                        id: indicator
                        x: padding + (depth * indentation)
                        anchors.verticalCenter: parent.verticalCenter
                        visible: isTreeNode // && hasChildren
                        text: expanded ? "▾" : "▸"

                        TapHandler {
                            onSingleTapped: {
                                let index = treeView.index(row, column)
                                treeView.selectionModel.setCurrentIndex(index, ItemSelectionModel.NoUpdate)
                                treeView.toggleExpanded(row)
                            }
                        }
                    }

                    Item {
                        width: styleData.depth * 14
                        height: 1
                    }

                    CheckButton {
                        id: toggleBtn
                        checkable: false
                        visible: model.isFolder
                        iconSource: rowExpanded ? "qrc:/icons/ui/toggle_left.svg" : "qrc:/icons/ui/toggle_right.svg"
                        implicitWidth: theme.controlHeight * 0.9
                        implicitHeight: theme.controlHeight * 0.9
                        onClicked: {
                            if (typeof tree.toggleExpanded === "function") {
                                tree.toggleExpanded(styleData.index)
                            } else if (typeof tree.isExpanded === "function") {
                                if (tree.isExpanded(styleData.index)) {
                                    tree.collapse(styleData.index)
                                } else {
                                    tree.expand(styleData.index)
                                }
                            }
                            if (geo) geo.toggleFolderExpanded(model.id)
                        }

                        property bool rowExpanded: (typeof tree.isExpanded === "function")
                                                   ? tree.isExpanded(styleData.index)
                                                   : model.expanded
                    }

                    Item {
                        width: toggleBtn.visible ? 0 : (theme.controlHeight * 0.9)
                        height: 1
                        visible: !toggleBtn.visible
                    }

                    Label {
                        text: model.isFolder ? model.name : model.geomType
                        color: "white"
                        elide: Label.ElideRight
                        Layout.preferredWidth: 140
                    }

                    Label {
                        text: "(" + model.vertexCount + ")"
                        color: "#cfcfcf"
                        Layout.preferredWidth: 48
                        horizontalAlignment: Text.AlignRight
                    }

                    Label {
                        text: model.id
                        color: "#9a9a9a"
                        elide: Label.ElideRight
                        Layout.fillWidth: true
                    }

                    CheckButton {
                        checkable: true
                        text: "V"
                        checked: model.visible
                        onClicked: if (geo) geo.setNodeVisible(model.id, model.isFolder, checked)
                    }
                }

                // MouseArea {
                //     anchors.fill: parent
                //     acceptedButtons: Qt.LeftButton
                //     onClicked: {
                //         tree.currentIndex = styleData.index
                //         if (geo) geo.selectNode(model.id, model.isFolder, model.parentId)
                //     }
                // }
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
