import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

ParamGroup {
    groupName: "CSV import"
    visible: importCheck.checked
    Layout.margins: 24

    RowLayout {
        ParamSetup {
            paramName: "Separator: "
            Layout.fillWidth: true

            CCombo  {
                id: separatorCombo
                //                    Layout.fillWidth: true
                model: ["Comma", "Tab", "Space", "SemiColon"]

                Settings {
                    property alias separatorCombo: separatorCombo.currentIndex
                }
            }
        }

        ParamSetup {
            paramName: "Row: "
            Layout.fillWidth: true

            SpinBoxCustom {
                id:firstRow
                implicitWidth: 100
                from: 1
                to: 100
                stepSize: 1
                value: 1

                Settings {
                    property alias importCSVfirstRow: firstRow.value
                }
            }
        }
    }


    RowLayout {
        CCheck {
            id: timeEnable
            //                    Layout.fillWidth: true
            //                        Layout.preferredWidth: 150
            checked: true
            text: "Time"

            Settings {
                property alias importCSVtimeEnable: timeEnable.checked
            }
        }

        //                CTextField {
        //                    id: timeFormater
        //                    text: "yyyy-MM-dd hh:mm:ss,zzz"
        //                    Settings {
        //                        property alias importCSVtimeFormater: timeFormater.text
        //                    }
        //                }

        SpinBoxCustom {
            id:timeColumn
            implicitWidth: 100
            from: 1
            to: 100
            stepSize: 1
            value: 6

            Settings {
                property alias importCSVtimeColumn: timeColumn.value
            }
        }

        CCombo  {
            id: utcGpsCombo
            //                    Layout.fillWidth: true
            model: ["UTC time", "GPS time"]

            Settings {
                property alias utcGpsCombo: utcGpsCombo.currentIndex
            }
        }
    }

    //            RowLayout {
    //                CCheck {
    //                    id: utcTime
    //                    Layout.fillWidth: true
    //                    checked: true
    //                    text: "UTC time"

    //                    Settings {
    //                        property alias utcTime: utcTime.checked
    //                    }
    //                }

    //            }


    RowLayout {
        CCheck {
            id: latLonEnable
            Layout.fillWidth: true
            //                        Layout.preferredWidth: 150
            checked: true
            text: "Lat/Lon/Alt"

            Settings {
                property alias importCSVlatLonEnable: latLonEnable.checked
            }
        }

        SpinBoxCustom {
            id: latColumn
            implicitWidth: 100
            from: 1
            to: 100
            stepSize: 1
            value: 2

            Settings {
                property alias importCSVlatColumn: latColumn.value
            }
        }

        SpinBoxCustom {
            id: lonColumn
            implicitWidth: 100
            from: 1
            to: 100
            stepSize: 1
            value: 3

            Settings {
                property alias importCSVlonColumn: lonColumn.value
            }
        }

        SpinBoxCustom {
            id: altColumn
            implicitWidth: 100
            from: 1
            to: 100
            stepSize: 1
            value: 4

            Settings {
                property alias importCSValtColumn: altColumn.value
            }
        }
    }



    RowLayout {
        CCheck {
            id: xyzEnable
            Layout.fillWidth: true
            //                        Layout.preferredWidth: 150
            checked: true
            text: "NEU"

            Settings {
                property alias importCSVxyzEnable: xyzEnable.checked
            }
        }

        SpinBoxCustom {
            id: northColumn
            implicitWidth: 100
            from: 1
            to: 100
            stepSize: 1
            value: 2

            Settings {
                property alias importCSVnorthColumn: northColumn.value
            }
        }

        SpinBoxCustom {
            id: eastColumn
            implicitWidth: 100
            from: 1
            to: 100
            stepSize: 1
            value: 3

            Settings {
                property alias importCSVeastColumn: eastColumn.value
            }
        }

        SpinBoxCustom {
            id: upColumn
            implicitWidth: 100
            from: 1
            to: 100
            stepSize: 1
            value: 4

            Settings {
                property alias importCSVupColumn: upColumn.value
            }
        }
    }



    RowLayout {
        CTextField {
            id: importPathText
            hoverEnabled: true
            Layout.fillWidth: true
            //                    visible: connectionTypeCombo.currentText === "File"

            text: ""
            placeholderText: qsTr("Enter path")

            Keys.onPressed: {
                if (event.key === 16777220) {
                    importTrackFileDialog.openCSV();
                }
            }

            Settings {
                property alias importPathText: importPathText.text
            }
        }

        CButton {
            text: "..."
            Layout.fillWidth: false
            visible: connectionTypeCombo.currentText === "File"
            implicitHeight: theme.controlHeight
            implicitWidth: implicitHeight*1.1
            onClicked: {
                importTrackFileDialog.open()
            }

            FileDialog {
                id: importTrackFileDialog
                title: "Please choose a file"
                folder: shortcuts.home
                //                    fileMode: FileDialog.OpenFiles

                nameFilters: ["Logs (*.csv *.txt)"]

                function openCSV() {
                    core.openCSV(importPathText.text, separatorCombo.currentIndex, firstRow.value, timeColumn.value, utcGpsCombo.currentIndex == 0,
                                 latColumn.value*latLonEnable.checked, lonColumn.value*latLonEnable.checked, altColumn.value*latLonEnable.checked,
                                 northColumn.value*xyzEnable.checked, eastColumn.value*xyzEnable.checked, upColumn.value*xyzEnable.checked);
                }

                onAccepted: {
                    importPathText.text = importTrackFileDialog.fileUrl.toString()

                    openCSV();
                }
                onRejected: {
                }
            }

            Settings {
                property alias importFolder: importTrackFileDialog.folder
            }
        }
    }

}
