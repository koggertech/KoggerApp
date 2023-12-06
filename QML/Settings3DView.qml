import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1
import QtQuick.Controls.Styles 1.4

MenuScroll {

    //    Layout.fillWidth: true
    //    Layout.preferredHeight: settings3DLayout.height

    property bool displayScene: displaySceneCheckBox.checked
    property color contourColor: Scene3DModel.contourColor()
    property color contourKeyPointsColor: Scene3DModel.contourKeyPointsColor()


    ColumnLayout {
        id: settings3DLayout
        width: parent.width
        Layout.preferredWidth: parent.width
        Layout.maximumWidth: parent.width
        implicitWidth:  parent.width


        ColumnLayout {
            Layout.maximumWidth: parent.width
            Layout.preferredWidth: parent.width

            MenuBlock {
            }

            ColumnLayout {
                Layout.margins: 24
                spacing: 24

                ParamGroup {
                    groupName: "Scene"

                    CCheck {
                        id: displayTrackCheckBox
                        Layout.fillWidth: true
                        checked: true
                        text: "Bottom track"
                        onCheckedChanged: Settings3DController.changeBottomTrackVisibility(checked)
                        Component.onCompleted: Settings3DController.changeBottomTrackVisibility(checked)

                        Settings {
                            property alias displayTrackCheckBox: displayTrackCheckBox.checked
                        }
                    }

                    CCheck {
                        id: displaySurfaceCheckBox
                        Layout.fillWidth: true
                        checked: false
                        text: "Surface"
                        onCheckedChanged: Settings3DController.changeSurfaceVisibility(checked)
                        Component.onCompleted: Settings3DController.changeSurfaceVisibility(checked)

                        Settings {
                            property alias displaySurfaceCheckBox: displaySurfaceCheckBox.checked
                        }
                    }

                    CCheck {
                        id: displayGridCheckBox
                        Layout.fillWidth: true
                        checked: false
                        text: "Surface's grid"
                        onCheckedChanged: Settings3DController.changeSurfaceGridVisibility(checked)
                        Component.onCompleted: Settings3DController.changeSurfaceGridVisibility(checked)

                        Settings {
                            property alias displayGridCheckBox: displayGridCheckBox.checked
                        }
                    }

                    RowLayout {
                        CCheck {
                            id: displayContourCheckBox
                            Layout.fillWidth: true
                            checked: false
                            text: "Surface's contour"
                            onCheckedChanged: Settings3DController.changeContourVisibility(checked)
                            Component.onCompleted: Settings3DController.changeContourVisibility(checked)

                            Settings {
                                property alias displayContourCheckBox: displayContourCheckBox.checked
                            }
                        }

                        CButton {
                            id: contourColorButton
                            onClicked: colorDialog.open()
                            text:""
                            Layout.preferredWidth: 40
                            Layout.minimumWidth: 40
                            Layout.fillWidth: false
//                            Layout.alignment: Qt.AlignRight

                            background: Rectangle {
                                color: contourColor

                            }
                        }

                        ColorDialog {
                            id: colorDialog
                            title: "Contour color picker"
                            onAccepted: Settings3DController.changeContourColor(currentColor)
                        }
                    }

                    RowLayout {
                        CCheck{
                            id: displayControlPointsCheckBox
                            Layout.fillWidth: true
                            checked: false
                            text: "Contour points"
                            onCheckedChanged: Settings3DController.changeContourKeyPointsVisibility(checked)
                            Component.onCompleted: Settings3DController.changeContourKeyPointsVisibility(checked)

                            Settings {
                                property alias displayControlPointsCheckBox: displayControlPointsCheckBox.checked
                            }
                        }

                        CButton {
                            id: contourKeyPointsColorButton
                            onClicked: contourKeyPointsColorDialog.open()
                            text:""
                            Layout.maximumWidth: 40
                            Layout.minimumWidth: 40
                            Layout.fillWidth: false
//                            Layout.alignment: Qt.AlignRight


                            background: Rectangle {
                                color: contourKeyPointsColor
                            }
                        }


                        ColorDialog {
                            id: contourKeyPointsColorDialog
                            title: "Contour key points color picker"
                            onAccepted: Settings3DController.changeContourKeyPointsColor(currentColor)
                        }
                    }

                    //            ParamSetup {
                    //                paramName: "Line width: "

                    //                Slider {
                    //                    id: lineWidthSlider
                    //                    Layout.fillWidth: true
                    //                    stepSize: 1.0
                    //                    from: 1.0
                    //                    value: 6.0
                    //                    to: 10.0
                    //                    onValueChanged: Settings3DController.changeContourLineWidth(value)
                    //                }
                    //            }


                    ParamSetup {
                        paramName: "Vertical scale: "

                        Slider {
                            id: verticalScaleSlider
                            Layout.fillWidth: true
                            stepSize: 0.1
                            from: 1.0
                            value: 1.0
                            to: 10.0
                            onValueChanged: renderer.setModelScaleZ(value)
                            Component.onCompleted: renderer.setModelScaleZ(value)
                        }
                    }

                }

                ParamGroup {
                    groupName: "Surface processing"

                    ParamSetup {
                        paramName: "Calculation method:"
                        CCombo  {
                            id: calcMethodCCombo
                            Layout.fillWidth: true
                            model: ["TIN"]
                            enabled:Scene3DModel.triangulationAvailable()
                            currentIndex: 0
                            onCurrentTextChanged: Settings3DController.changeCalculationMethod(currentText)
                            Component.onCompleted: Settings3DController.changeCalculationMethod(currentText)

                            contentItem: Text {
                                font: theme.textFont
                                text: calcMethodCCombo.currentText
                                color: enabled ? theme.textColor : theme.disabledTextColor
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                color: {
                                    !calcMethodCCombo.enabled ? theme.disabledBackColor :
                                                                calcMethodCCombo.hovered ? theme.hoveredBackColor  :
                                                                                           theme.controlBackColor
                                }
                            }
                        }
                    }

                    // Параметры TIN.
                    // TODO: Реализовать более гибкую смену отображаемых параметров
                    // метода расчета поверхности, т.к. фактически методов расчета
                    // может быть больше
                    ParamSetup {
                        paramName: "TIN edge length limit:"
                        visible: calcMethodCCombo.currentText == "TIN"

                        SpinBoxCustom {
                            id: maxTriEdgeLengthSpinBox
                            from: 100
                            to: 1000000
                            stepSize: 100
                            value: 40000

                            property int decimals: 2
                            property real realValue: value / 1000

                            validator: DoubleValidator {
                                bottom: Math.min(from, to)
                                top:  Math.max(from, to)
                            }

                            textFromValue: function(value, locale) {
                                return Number(value / 1000).toLocaleString(locale, 'f', decimals)
                            }

                            valueFromText: function(text, locale) {
                                return Number.fromLocaleString(locale, text) * 1000
                            }

                            onRealValueChanged: Settings3DController.setTriangulationEdgeLengthLimit(value / 1000)
                            Component.onCompleted: Settings3DController.setTriangulationEdgeLengthLimit(value / 1000)

                            Settings {
                                property alias maxTriEdgeLengthSpinBox: maxTriEdgeLengthSpinBox.value
                            }
                        }
                    }



                    ParamSetup {
                        paramName: "Smooth method:"
                        id: smoothMetodParam

                        CCombo  {
                            id: smoothMethodCCombo
                            Layout.fillWidth: true
                            model: ["None","Barycentric"]
                            enabled:Scene3DModel.triangulationAvailable()
                            currentIndex: 1
                            onCurrentTextChanged: Settings3DController.changeSmoothingMethod(currentText)
                            Component.onCompleted: Settings3DController.changeSmoothingMethod(currentText)

                            contentItem: Text {
                                font: theme.textFont
                                text: smoothMethodCCombo.currentText
                                color: enabled ? theme.textColor : theme.disabledTextColor
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                color: {
                                    !smoothMethodCCombo.enabled ? theme.disabledBackColor :
                                                                  smoothMethodCCombo.hovered ? theme.hoveredBackColor  :
                                                                                               theme.controlBackColor
                                }
                            }

                            Settings {
                                property alias smoothMethodCCombo: smoothMethodCCombo.currentIndex
                            }
                        }
                    }

                    ParamSetup {
                        paramName: "Grid type:"
                        visible: smoothMethodCCombo.currentText != "None"

                        CCombo  {
                            id: gridTypeCombo
                            Layout.fillWidth: true
                            model: ["Quad"]
                            enabled:Scene3DModel.triangulationAvailable()
                            currentIndex: 0
                            onCurrentTextChanged: Settings3DController.changeGridType(currentText)
                            Component.onCompleted: Settings3DController.changeGridType(currentText)

                            contentItem: Text {
                                font: theme.textFont
                                text: gridTypeCombo.currentText
                                color: enabled ? theme.textColor : theme.disabledTextColor
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                color: {
                                    !gridTypeCombo.enabled ? theme.disabledBackColor :
                                                             gridTypeCombo.hovered ? theme.hoveredBackColor  :
                                                                                     theme.controlBackColor
                                }
                            }

                            Settings {
                                property alias gridTypeCombo: gridTypeCombo.currentIndex
                            }
                        }
                    }

                    ParamSetup {
                        paramName: "Grid cell size:"

                        visible: gridTypeCombo.visible

                        SpinBoxCustom {
                            id: gridCellSize
                            from: 100
                            to: 100000
                            stepSize: 100
                            value: 4000

                            property int decimals: 2
                            property real realValue: value / 1000

                            validator: DoubleValidator {
                                bottom: Math.min(from, to)
                                top:  Math.max(from, to)
                            }

                            textFromValue: function(value, locale) {
                                return Number(value / 1000).toLocaleString(locale, 'f', decimals)
                            }

                            valueFromText: function(text, locale) {
                                return Number.fromLocaleString(locale, text) * 1000
                            }

                            onRealValueChanged: Settings3DController.changeGridCellSize(value / 1000)
                            Component.onCompleted: Settings3DController.changeGridCellSize(value / 1000)

                            Settings {
                                property alias gridCellSize: gridCellSize.value
                            }
                        }
                    }

                    CButton {
                        id: updateSurfaceButton
                        Layout.fillWidth: true
                        enabled: Scene3DModel.triangulationAvailable()
                        onClicked: {
                            if (active)
                                Settings3DController.updateDisplayedObject()
                        }

                        contentItem: Text {
                            font: theme.textFont
                            text: "Update surface"
                            color: enabled ? theme.textColor : theme.disabledTextColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

//                        background: Rectangle {
//                            color: {
//                                !updateSurfaceButton.enabled ? theme.disabledBackColor :
//                                                               updateSurfaceButton.hovered ? theme.hoveredBackColor  :
//                                                                                             theme.controlBackColor
//                            }
//                        }
                    }

                }
            }

        }
    }

    Connections {
        target: Scene3DModel
        onStateChanged: {
            displayTrackCheckBox.enabled     = Scene3DModel.triangulationAvailable()
            displaySurfaceCheckBox.enabled   = Scene3DModel.triangulationAvailable()
            displayGridCheckBox.enabled      = Scene3DModel.triangulationAvailable()
            interpLevelCCombo.enabled        = Scene3DModel.triangulationAvailable()
            updateSurfaceButton.enabled      = Scene3DModel.triangulationAvailable()
            maxTriEdgeLengthSpinBox.enabled  = Scene3DModel.triangulationAvailable()
            calcMethodCCombo.enabled         = Scene3DModel.triangulationAvailable()
            //maxTriEdgeLengthSpinBox.from     = Scene3DModel.minTriEdgeLength()
            //maxTriEdgeLengthSpinBox.to       = Scene3DModel.maxTriEdgeLength()
            //maxTriEdgeLengthSpinBox.stepSize = Math.abs(maxTriEdgeLengthSpinBox.to-maxTriEdgeLengthSpinBox.from) / 20;
        }

        onContourPropertiesChanged: {
            contourColor = Scene3DModel.contourColor()
            contourKeyPointsColor = Scene3DModel.contourKeyPointsColor()
        }

    }
}
