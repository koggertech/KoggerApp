import QtQuick 2.15
import QtQuick.Layouts 1.15

MenuFrame {
    id: extraInfoPanel

    readonly property var fallbackMenuBar: ({
        extraInfoVis: false,
        extraInfoCoordinatesVis: false,
        extraInfoDepthVis: false,
        extraInfoSpeedVis: false,
        extraInfoActivePointVis: false,
        extraInfoSimpleNavV2Vis: false,
        extraInfoBoatStatusVis: false
    })
    readonly property var fallbackDataset: ({
        isBoatCoordinateValid: false,
        isLastDepthValid: false,
        isSpeedValid: false,
        isActiveContactIndxValid: false,
        isSimpleNavV2Valid: false,
        isBoatStatusValid: false,
        boatLatitude: 0,
        boatLongitude: 0,
        distToContact: 0,
        angleToContact: 0,
        depth: 0,
        speed: 0,
        simpleNavV2GnssFixType: 0,
        simpleNavV2NumSats: 0,
        simpleNavV2UnixTime: 0,
        simpleNavV2UnixOffsetMs: 0,
        simpleNavV2Latitude: 0,
        simpleNavV2Longitude: 0,
        simpleNavV2GroundCourseDeg: 0,
        simpleNavV2GroundVelocityMps: 0,
        simpleNavV2YawDeg: 0,
        simpleNavV2PitchDeg: 0,
        simpleNavV2RollDeg: 0,
        boatStatusBatteryBoatPercent: 0,
        boatStatusBatteryBridgePercent: 0,
        boatStatusSignalQualityBoatPercent: 0,
        boatStatusSignalQualityBridgePercent: 0
    })

    property var menuBarState: null
    property var datasetState: null
    property bool showBanner: false
    readonly property var mb: menuBarState ? menuBarState : fallbackMenuBar
    readonly property var ds: datasetState ? datasetState : fallbackDataset

    visible: mb.extraInfoVis
             && !showBanner
             && ((mb.extraInfoCoordinatesVis && ds.isBoatCoordinateValid)
                 || (mb.extraInfoDepthVis && ds.isLastDepthValid)
                 || (mb.extraInfoSpeedVis && ds.isSpeedValid)
                 || (mb.extraInfoActivePointVis && ds.isActiveContactIndxValid)
                 || (mb.extraInfoSimpleNavV2Vis && ds.isSimpleNavV2Valid)
                 || (mb.extraInfoBoatStatusVis && ds.isBoatStatusValid))

    isDraggable: true
    isOpacityControlled: true
    horizontalMargins: 12
    verticalMargins: 10
    spacing: 8

    function lpad(s, w, ch) {
        s = String(s)
        while (s.length < w) s = (ch || ' ') + s
        return s
    }
    function formatFixed(value, fracDigits, intWidth) {
        if (!isFinite(value)) return lpad("-", intWidth + 1 + fracDigits)
        var sign = value < 0 ? "-" : " "
        var abs  = Math.abs(value)
        var s    = abs.toFixed(fracDigits)
        var p    = s.split(".")
        var intP = lpad(p[0], intWidth, " ")
        return sign + intP + (fracDigits > 0 ? "." + p[1] : "")
    }
    function toDMS(value, isLat) {
        var hemi = isLat ? (value >= 0 ? "N" : "S") : (value >= 0 ? "E" : "W");
        var abs  = Math.abs(value)
        var s    = abs.toFixed(4)
        var p    = s.split(".")
        var intP = lpad(p[0], 3, " ")
        return hemi + " " + intP + "." + p[1]
    }

    property string latDms: ""
    property string lonDms: ""
    property string distStr: ""
    property string angStr: ""
    property string depthStr: ""
    property string speedStr: ""

    function updateFields() {
        latDms   = toDMS(ds.boatLatitude,  true)  + qsTr("\u00B0")
        lonDms   = toDMS(ds.boatLongitude, false) + qsTr("\u00B0")
        distStr  = formatFixed(ds.distToContact, 1, 3) + qsTr(" m")
        angStr   = formatFixed(ds.angleToContact, 1, 3) + qsTr("\u00B0")
        depthStr = formatFixed(ds.depth, 2, 4) + qsTr(" m")
        speedStr = formatFixed(ds.speed, 1, 3) + qsTr(" km/h")
    }

    Timer {
        interval: 40
        repeat: true
        running: extraInfoPanel.visible
        triggeredOnStart: true
        onTriggered: extraInfoPanel.updateFields()
    }

    ColumnLayout {
        spacing: 6

        ColumnLayout {

            CText {
                visible: mb.extraInfoDepthVis && ds.isLastDepthValid
                text: extraInfoPanel.depthStr
                font.bold: true
                font.pixelSize: 40 * theme.resCoeff
                font.family: "monospace"
                leftPadding: 4
            }

            CText {
                visible: mb.extraInfoSpeedVis && ds.isSpeedValid
                text: extraInfoPanel.speedStr
                font.bold: true
                font.pixelSize: 40 * theme.resCoeff
                font.family: "monospace"
                leftPadding: 4
            }
        }

        ColumnLayout {
            visible: mb.extraInfoCoordinatesVis && ds.isBoatCoordinateValid

            CText {
                text: qsTr("Boat position")
                leftPadding: 4
                rightPadding: 4
                font.bold: true
                font.pixelSize: 16 * theme.resCoeff
            }

            RowLayout {
                spacing: 6
                CText { text: qsTr("Lat.:"); opacity: 0.7; leftPadding: 4; }
                Item  { Layout.fillWidth: true }
                CText { text: extraInfoPanel.latDms; }
            }

            RowLayout {
                spacing: 6
                CText { text: qsTr("Lon.:"); opacity: 0.7; leftPadding: 4; }
                Item  { Layout.fillWidth: true }
                CText { text: extraInfoPanel.lonDms; }
            }
        }

        ColumnLayout {
            visible: mb.extraInfoActivePointVis && ds.isActiveContactIndxValid

            CText {
                text: qsTr("Active point")
                leftPadding: 4
                rightPadding: 4
                font.bold: true
                font.pixelSize: 16 * theme.resCoeff
            }

            RowLayout {
                spacing: 6
                CText { text: qsTr("Dist.:"); opacity: 0.7; leftPadding: 4 }
                Item  { Layout.fillWidth: true }
                CText { text: extraInfoPanel.distStr; }
            }

            RowLayout {
                spacing: 6
                CText { text: qsTr("Ang.:"); opacity: 0.7; leftPadding: 4 }
                Item  { Layout.fillWidth: true }
                CText { text: extraInfoPanel.angStr; }
            }
        }

        Rectangle {
            visible: mb.extraInfoSimpleNavV2Vis && ds.isSimpleNavV2Valid
            Layout.fillWidth: true
            implicitHeight: simpleNavV2Layout.implicitHeight + 16
            radius: 6
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.25)
            color: Qt.rgba(1, 1, 1, 0.05)

            ColumnLayout {
                id: simpleNavV2Layout
                anchors.fill: parent
                anchors.margins: 8
                spacing: 4

                CText {
                    text: qsTr("SimpleNavV2")
                    font.bold: true
                    font.pixelSize: 16 * theme.resCoeff
                }

                RowLayout {
                    spacing: 6
                    CText { text: "gnss_fix_type"; opacity: 0.7 }
                    Item { Layout.fillWidth: true }
                    CText { text: String(ds.simpleNavV2GnssFixType) }
                }

                RowLayout {
                    spacing: 6
                    CText { text: "numSats"; opacity: 0.7 }
                    Item { Layout.fillWidth: true }
                    CText { text: String(ds.simpleNavV2NumSats) }
                }

                RowLayout {
                    spacing: 6
                    CText { text: "unix_time"; opacity: 0.7 }
                    Item { Layout.fillWidth: true }
                    CText { text: String(ds.simpleNavV2UnixTime) }
                }

                RowLayout {
                    spacing: 6
                    CText { text: "unix_offset_ms"; opacity: 0.7 }
                    Item { Layout.fillWidth: true }
                    CText { text: String(ds.simpleNavV2UnixOffsetMs) }
                }

                RowLayout {
                    spacing: 6
                    CText { text: "latitude"; opacity: 0.7 }
                    Item { Layout.fillWidth: true }
                    CText { text: Number(ds.simpleNavV2Latitude).toFixed(7) }
                }

                RowLayout {
                    spacing: 6
                    CText { text: "longitude"; opacity: 0.7 }
                    Item { Layout.fillWidth: true }
                    CText { text: Number(ds.simpleNavV2Longitude).toFixed(7) }
                }

                RowLayout {
                    spacing: 6
                    CText { text: "ground_course_deg"; opacity: 0.7 }
                    Item { Layout.fillWidth: true }
                    CText { text: Number(ds.simpleNavV2GroundCourseDeg).toFixed(2) }
                }

                RowLayout {
                    spacing: 6
                    CText { text: "ground_velocity_mps"; opacity: 0.7 }
                    Item { Layout.fillWidth: true }
                    CText { text: Number(ds.simpleNavV2GroundVelocityMps).toFixed(3) }
                }

                RowLayout {
                    spacing: 6
                    CText { text: "yaw_deg"; opacity: 0.7 }
                    Item { Layout.fillWidth: true }
                    CText { text: Number(ds.simpleNavV2YawDeg).toFixed(2) }
                }

                RowLayout {
                    spacing: 6
                    CText { text: "pitch_deg"; opacity: 0.7 }
                    Item { Layout.fillWidth: true }
                    CText { text: Number(ds.simpleNavV2PitchDeg).toFixed(2) }
                }

                RowLayout {
                    spacing: 6
                    CText { text: "roll_deg"; opacity: 0.7 }
                    Item { Layout.fillWidth: true }
                    CText { text: Number(ds.simpleNavV2RollDeg).toFixed(2) }
                }
            }
        }

        Rectangle {
            visible: mb.extraInfoBoatStatusVis && ds.isBoatStatusValid
            Layout.fillWidth: true
            implicitHeight: boatStatusLayout.implicitHeight + 16
            radius: 6
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.25)
            color: Qt.rgba(1, 1, 1, 0.05)

            ColumnLayout {
                id: boatStatusLayout
                anchors.fill: parent
                anchors.margins: 8
                spacing: 4

                CText {
                    text: qsTr("IDBinBoatStatus")
                    font.bold: true
                    font.pixelSize: 16 * theme.resCoeff
                }

                RowLayout {
                    spacing: 6
                    CText { text: "batteryBoat_percent"; opacity: 0.7 }
                    Item { Layout.fillWidth: true }
                    CText { text: String(ds.boatStatusBatteryBoatPercent) + "%" }
                }

                RowLayout {
                    spacing: 6
                    CText { text: "batteryBridge_percent"; opacity: 0.7 }
                    Item { Layout.fillWidth: true }
                    CText { text: String(ds.boatStatusBatteryBridgePercent) + "%" }
                }

                RowLayout {
                    spacing: 6
                    CText { text: "signal_quality_boat_percent"; opacity: 0.7 }
                    Item { Layout.fillWidth: true }
                    CText { text: String(ds.boatStatusSignalQualityBoatPercent) + "%" }
                }

                RowLayout {
                    spacing: 6
                    CText { text: "signal_quality_bridge_percent"; opacity: 0.7 }
                    Item { Layout.fillWidth: true }
                    CText { text: String(ds.boatStatusSignalQualityBridgePercent) + "%" }
                }
            }
        }
    }
}
