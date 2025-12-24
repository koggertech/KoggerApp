import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Dialogs

Item {
    id: root
    property var geo: null
    property var view: null

    implicitWidth: 380
    implicitHeight: 460

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
        anchors.fill: parent
        radius: 8
        color: "#1e1e1e"
        border.color: "#3a3a3a"
        border.width: 1
        opacity: 0.92
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
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

            Item { Layout.fillWidth: true }

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

            Item { Layout.fillWidth: true }

            Label {
                text: geo ? ("Folder: " + geo.currentFolderName) : "Folder: -"
                color: "#cfcfcf"
                elide: Label.ElideRight
                Layout.fillWidth: true
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

            Item { Layout.fillWidth: true }

            CheckButton {
                checkable: false
                iconSource: "qrc:/icons/ui/x.svg"
                implicitWidth: theme.controlHeight * 1.2
                implicitHeight: theme.controlHeight * 1.2
                enabled: geo ? geo.selectedFeatureId !== "" : false
                onClicked: if (geo) geo.deleteSelectedFeature()
            }
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 4
            model: geo ? geo.treeModel : null

            delegate: Rectangle {
                id: nodeItem
                width: list.width
                height: row.implicitHeight + 10
                radius: 6
                color: (geo && geo.selectedNodeId === model.id) ? "#2b4f7a" : "#2a2a2a"
                border.color: "#3a3a3a"
                border.width: 1

                RowLayout {
                    id: row
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 6

                    Item {
                        width: model.depth * 14
                        height: 1
                    }

                    CheckButton {
                        id: toggleBtn
                        checkable: false
                        visible: model.isFolder
                        iconSource: model.expanded ? "qrc:/icons/ui/toggle_left.svg" : "qrc:/icons/ui/toggle_right.svg"
                        implicitWidth: theme.controlHeight * 0.9
                        implicitHeight: theme.controlHeight * 0.9
                        onClicked: if (geo) geo.toggleFolderExpanded(model.id)
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

                MouseArea {
                    anchors.fill: parent
                    z: -1
                    onClicked: if (geo) geo.selectNode(model.id, model.isFolder, model.parentId)
                }
            }
        }

        Rectangle {
            visible: geo ? geo.selectedFeatureId !== "" : false
            color: "#262626"
            radius: 6
            border.color: "#3a3a3a"
            border.width: 1
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
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
