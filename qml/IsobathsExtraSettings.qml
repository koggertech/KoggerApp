import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Dialogs
import QtCore


// isobaths extra settings
MenuFrame {
    id: isobathsSettings

    property CheckButton isobathsCheckButton

    visible: Qt.platform.os === "android"
             ? (isobathsCheckButton.isobathsLongPressTriggered || isobathsTheme.activeFocus)
             : (isobathsCheckButton.hovered                    ||
                isHovered                                      ||
                isobathsTheme.activeFocus)

    z: isobathsSettings.visible
    Layout.alignment: Qt.AlignCenter

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
        }
    }

    onFocusChanged: {
        if (Qt.platform.os === "android" && !focus) {
            isobathsCheckButton.isobathsLongPressTriggered = false
        }
    }

    ColumnLayout {
        RowLayout {
            spacing: 16

            Rectangle {
                Layout.fillWidth: true
                height: 2
                color: "#808080"
            }

            CText {
                text: qsTr("Isobaths settings")
            }

            Rectangle {
                Layout.fillWidth: true
                height: 2
                color: "#808080"
            }
        }
        RowLayout {
            //visible: !isobathsDebugModeCheckButton.checked
            CText {
                text: qsTr("Theme:")
            }
            Item {
                Layout.fillWidth: true
            }
            CCombo  {
                id: isobathsTheme
                Layout.preferredWidth: 200
                model: [qsTr("Midnight"), qsTr("Default"), qsTr("Blue"), qsTr("Sepia"), qsTr("WRGBD"), qsTr("WhiteBlack"), qsTr("Standard")]
                currentIndex: 0
                onCurrentIndexChanged: {
                    IsobathsViewControlMenuController.onThemeChanged(currentIndex)
                }

                onFocusChanged: {
                    if (Qt.platform.os === 'android') {
                        isobathsSettings.focus = true
                    }
                }

                Component.onCompleted: {
                    IsobathsViewControlMenuController.onThemeChanged(currentIndex)
                }

                Settings {
                    property alias isobathsTheme: isobathsTheme.currentIndex
                }
            }
        }

        RowLayout {
            CText {
                text: qsTr("Edge limit, m:")
                Layout.fillWidth: true
            }
            SpinBoxCustom {
                id: isobathsEdgeLimitSpinBox
                implicitWidth: 200
                from: 10
                to: 1000
                stepSize: 5
                value: 20
                editable: false

                property int decimals: 1

                onFocusChanged: {
                    isobathsSettings.focus = true
                }

                Component.onCompleted: {
                    IsobathsViewControlMenuController.onEdgeLimitChanged(isobathsEdgeLimitSpinBox.value)
                }

                onValueChanged: {
                    IsobathsViewControlMenuController.onEdgeLimitChanged(isobathsEdgeLimitSpinBox.value)
                }

                Settings {
                    property alias isobathsEdgeLimitSpinBox: isobathsEdgeLimitSpinBox.value
                }
            }
        }

        RowLayout {
            //visible: !isobathsDebugModeCheckButton.checked

            CText {
                text: qsTr("Step, m:")
                Layout.fillWidth: true

            }
            SpinBoxCustom {
                id: isobathsSurfaceLineStepSizeSpinBox
                implicitWidth: 200
                from: 1
                to: 200
                stepSize: 1
                value: 10
                editable: false

                property int decimals: 1
                property real realValue: value / 10

                validator: DoubleValidator {
                    bottom: Math.min(isobathsSurfaceLineStepSizeSpinBox.from, isobathsSurfaceLineStepSizeSpinBox.to)
                    top:  Math.max(isobathsSurfaceLineStepSizeSpinBox.from, isobathsSurfaceLineStepSizeSpinBox.to)
                }

                textFromValue: function(value, locale) {
                    return Number(value / 10).toLocaleString(locale, 'f', decimals)
                }

                valueFromText: function(text, locale) {
                    return Number.fromLocaleString(locale, text) * 10
                }

                onFocusChanged: {
                    isobathsSettings.focus = true
                }

                Component.onCompleted: {
                    IsobathsViewControlMenuController.onSetSurfaceLineStepSize(isobathsSurfaceLineStepSizeSpinBox.realValue)
                }

                onRealValueChanged: {
                    IsobathsViewControlMenuController.onSetSurfaceLineStepSize(isobathsSurfaceLineStepSizeSpinBox.realValue)
                }

                Settings {
                    property alias isobathsSurfaceLineStepSizeSpinBox: isobathsSurfaceLineStepSizeSpinBox.value
                }
            }
        }

        RowLayout {
            CText {
                text: qsTr("Extra width, m:")
            }
            Item {
                Layout.fillWidth: true
            }
            SpinBoxCustom {
                id: extraWidthSpinBox
                implicitWidth: 200
                from: 5
                to: 100
                stepSize: 5
                value: 5
                editable: false

                onFocusChanged: {
                    isobathsSettings.focus = true
                }

                onValueChanged: {
                    IsobathsViewControlMenuController.onSetExtraWidth(value)
                }
                Component.onCompleted: {
                    IsobathsViewControlMenuController.onSetExtraWidth(value)
                }

                Settings {
                    property alias extraWidthSpinBox: extraWidthSpinBox.value
                }
            }
        }

        RowLayout {
            CTextField {
                id: exportSurfacePathText
                hoverEnabled: true
                Layout.maximumWidth: 200
                Layout.fillWidth: true
                placeholderText: qsTr("Enter path")
            }

            CButton {
                text: "..."
                Layout.fillWidth: false

                onClicked: {
                    exportSurfaceFileDialog.open()
                }
            }

            FileDialog  {
                id: exportSurfaceFileDialog
                title: qsTr("Select folder and set .csv file name")

                currentFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)

                fileMode: FileDialog.SaveFile

                nameFilters: ["CSV Files (*.csv)", "All Files (*)"]
                defaultSuffix: "csv"

                onAccepted: {
                    var url = selectedFile.toString()

                    if (!url.toLowerCase().endsWith(".csv")) {
                        url += ".csv"
                    }

                    exportSurfacePathText.text = url
                }
            }

            CButton {
                text: qsTr("Export to CSV")
                Layout.fillWidth: true
                onClicked: Scene3DControlMenuController.onExportToCSVButtonClicked(exportSurfacePathText.text)
            }


            Settings {
                property alias exportSurfaceFolder: exportSurfaceFileDialog.currentFolder
            }

            Settings {
                property alias exportSurfaceFolderText: exportSurfacePathText.text
            }
        }

        // RowLayout {
        //     visible: !isobathsDebugModeCheckButton.checked

        //     CText {
        //         text: qsTr("Label step, m:")
        //         Layout.fillWidth: true
        //     }
        //     SpinBoxCustom {
        //         id: isobathsLabelStepSpinBox
        //         implicitWidth: 200
        //         from: 10
        //         to: 1000
        //         stepSize: 5
        //         value: 100
        //         editable: false

        //         property int decimals: 1

        //         onFocusChanged: {
        //             isobathsSettings.focus = true
        //         }

        //         Component.onCompleted: {
        //             IsobathsViewControlMenuController.onSetLabelStepSize(isobathsLabelStepSpinBox.value)
        //         }

        //         onValueChanged: {
        //             IsobathsViewControlMenuController.onSetLabelStepSize(isobathsLabelStepSpinBox.value)
        //         }

        //         Settings {
        //             property alias isobathsLabelStepSpinBox: isobathsLabelStepSpinBox.value
        //         }
        //     }
        // }

        // CheckButton {
        //     text: qsTr("Triangles")
        //     Layout.fillWidth: true
        //     checked: true
        //     visible: isobathsDebugModeCheckButton.checked

        //     onCheckedChanged: {
        //         IsobathsViewControlMenuController.onTrianglesVisible(checked);
        //     }

        //     onFocusChanged: {
        //         isobathsSettings.focus = true
        //     }
        // }

        // CheckButton {
        //     text: qsTr("Edges")
        //     Layout.fillWidth: true
        //     checked: true
        //     visible: isobathsDebugModeCheckButton.checked

        //     onCheckedChanged: {
        //         IsobathsViewControlMenuController.onEdgesVisible(checked);
        //     }

        //     onFocusChanged: {
        //         isobathsSettings.focus = true
        //     }
        // }

        // CheckButton {
        //     id: isobathsDebugModeCheckButton
        //     text: qsTr("Debug mode")
        //     Layout.fillWidth: true
        //     checked: false

        //     onCheckedChanged: {
        //         IsobathsViewControlMenuController.onDebugModeView(checked);
        //     }

        //     onFocusChanged: {
        //         isobathsSettings.focus = true
        //     }
        // }

        // CButton {
        //     id: resetIsobathsButton
        //     text: qsTr("Clear")
        //     Layout.fillWidth: true
        //     onClicked: {
        //         IsobathsViewControlMenuController.onResetIsobathsButtonClicked()
        //     }

        //     onFocusChanged: {
        //         isobathsSettings.focus = true
        //     }
        // }

        // CButton {
        //     id: updateIsobathsButton
        //     text: qsTr("Update")
        //     Layout.fillWidth: true

        //     onClicked: {
        //         //IsobathsViewControlMenuController.onResetIsobathsButtonClicked()
        //         IsobathsViewControlMenuController.onUpdateIsobathsButtonClicked()
        //     }

        //     onFocusChanged: {
        //         isobathsSettings.focus = true
        //     }
        // }
    }
}
