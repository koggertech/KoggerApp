import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3

ColumnLayout {
    property int channelNumber: 0
    RowLayout {
        CCheck {
            id:switchDatasetChart1
            checked: sonarDriver.datasetChart == channelNumber
            text: "Chart"

            onCheckStateChanged: {
                if(checked == true && sonarDriver.datasetChart != channelNumber) {
                    sonarDriver.datasetChart = channelNumber
                } else if(checked == false && sonarDriver.datasetChart == channelNumber) {
                    sonarDriver.datasetChart = 0
                }
            }
        }

        CCheck {
            id:switchDatasetDist1
            checked: sonarDriver.datasetDist == channelNumber
            text: "Dist. Bin"

            onCheckStateChanged: {
                if(checked == true && sonarDriver.datasetDist != channelNumber) {
                    sonarDriver.datasetDist = channelNumber
                } else if(checked == false && sonarDriver.datasetDist == channelNumber) {
                    sonarDriver.datasetDist = 0
                }
            }
        }

        CCheck {
            id:switchDatasetDistNMEA1
            checked: sonarDriver.datasetSDDBT == channelNumber
            text: "Dist. NMEA"

            onCheckStateChanged: {
                if(checked == true && sonarDriver.datasetSDDBT != channelNumber) {
                    sonarDriver.datasetSDDBT = channelNumber
                } else if(checked == false && sonarDriver.datasetSDDBT == channelNumber) {
                    sonarDriver.datasetSDDBT = 0
                }
            }
        }

        CCheck {
            id:switchDatasetDistNMEAP1
            checked: sonarDriver.datasetSDDBT_P2 == channelNumber
            text: "Dist. NMEA #2"

            onCheckStateChanged: {
                if(checked == true && sonarDriver.datasetSDDBT_P2 != channelNumber) {
                    sonarDriver.datasetSDDBT_P2 = channelNumber
                } else if(checked == false && sonarDriver.datasetSDDBT_P2 == channelNumber) {
                    sonarDriver.datasetSDDBT_P2 = 0
                }
            }
        }
    }

    RowLayout {
        CCheck {
            id:switchDatasetTempP1
            checked: sonarDriver.datasetTemp == channelNumber
            text: "Temperature"

            onCheckStateChanged: {
                if(checked == true && sonarDriver.datasetTemp != channelNumber) {
                    sonarDriver.datasetTemp = channelNumber
                } else if(sonarDriver.datasetTemp == channelNumber) {
                    sonarDriver.datasetTemp = 0
                }
            }
        }


    }
}
