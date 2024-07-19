import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ColumnLayout {
    property var dev: null

    property int channelNumber: 0
    RowLayout {
        CCheck {
            id:switchDatasetChart1
            checked: dev.datasetChart === channelNumber
            text: qsTr("Chart")

            onCheckStateChanged: {
                if(checked == true && dev.datasetChart !== channelNumber) {
                    dev.datasetChart = channelNumber
                } else if(checked == false && dev.datasetChart === channelNumber) {
                    dev.datasetChart = 0
                }
            }
        }

        CCheck {
            id:switchDatasetDist1
            checked: dev.datasetDist === channelNumber
            text: qsTr("Distance")

            onCheckStateChanged: {
                if(checked == true && dev.datasetDist !== channelNumber) {
                    dev.datasetDist = channelNumber
                } else if(checked == false && dev.datasetDist === channelNumber) {
                    dev.datasetDist = 0
                }
            }
        }

        CCheck {
            id:switchDatasetDistNMEA1
            checked: dev.datasetSDDBT === channelNumber
            text: qsTr("Dist. NMEA")

            onCheckStateChanged: {
                if(checked == true && dev.datasetSDDBT !== channelNumber) {
                    dev.datasetSDDBT = channelNumber
                } else if(checked == false && dev.datasetSDDBT === channelNumber) {
                    dev.datasetSDDBT = 0
                }
            }
        }

        CCheck {
            id:switchDatasetDistNMEAP1
            checked: dev.datasetSDDBT_P2 === channelNumber
            text: qsTr("Dist. NMEA#2")

            onCheckStateChanged: {
                if(checked == true && dev.datasetSDDBT_P2 !== channelNumber) {
                    dev.datasetSDDBT_P2 = channelNumber
                } else if(checked == false && dev.datasetSDDBT_P2 === channelNumber) {
                    dev.datasetSDDBT_P2 = 0
                }
            }
        }


        CCheck {
            id:switchDatasetTimestamp
            checked: dev.datasetTimestamp === channelNumber
            text: qsTr("Timestamp")

            onCheckStateChanged: {
                if(checked == true && dev.datasetTimestamp !== channelNumber) {
                    dev.datasetTimestamp = channelNumber
                } else if(checked == false && dev.datasetTimestamp === channelNumber) {
                    dev.datasetTimestamp = 0
                }
            }
        }

        CCheck {
            id:switchDatasetTemp
            checked: dev.datasetTemp === channelNumber
            text: qsTr("Temper.")

            onCheckStateChanged: {
                if(checked == true && dev.datasetTemp !== channelNumber) {
                    dev.datasetTemp = channelNumber
                } else if(checked == false && dev.datasetTemp === channelNumber) {
                    dev.datasetTemp = 0
                }
            }
        }
    }

    RowLayout {
        CCheck {
            id:switchDatasetAttYPR
            checked: dev.datasetEuler === channelNumber
            text: qsTr("Euler")

            onCheckStateChanged: {
                if(checked == true && dev.datasetEuler !== channelNumber) {
                    dev.datasetEuler = channelNumber
                } else if(checked == false && dev.datasetEuler === channelNumber) {
                    dev.datasetEuler = 0
                }
            }
        }
    }
}
