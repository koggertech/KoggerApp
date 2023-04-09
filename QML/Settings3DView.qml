import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1
import QtQuick.Controls.Styles 1.4

Item {

    Layout.fillWidth: true
    Layout.preferredHeight: settings3DLayout.height

    property bool displayScene: displaySceneCheckBox.checked
    property color contourColor: Scene3DModel.contourColor()
    property color contourKeyPointsColor: Scene3DModel.contourKeyPointsColor()

    MenuBlock {
    }

    ColumnLayout {
        id: settings3DLayout
        Layout.margins: 24
        width: parent.width

        ParamGroup {
            groupName: "Common"
            Layout.margins: 10

            CCheck {
                id: displaySceneCheckBox
                Layout.fillWidth: true
                checked: Scene3DModel.sceneVisibility()
                text: "Display 3D scene"
                onCheckedChanged: Settings3DController.changeSceneVisibility(checked)
            }
        }

        ParamGroup {
            groupName: "Bottom track"
            Layout.margins: 10
            spacing: 24


            CCheck {
                id: displayTrackCheckBox
                Layout.fillWidth: true
                checked: Scene3DModel.bottomTrackVisible()
                text: "Display bottom track"
                onCheckedChanged: Settings3DController.changeBottomTrackVisibility(checked)
            }
        }

        ParamGroup {
            groupName: "Surface"
            Layout.margins: 10
            spacing: 24

            RowLayout {
                    id: dispModeLayout
                    width: parent.width

                CCheck {
                    id: displaySurfaceCheckBox
                    Layout.fillWidth: true
                    checked: Scene3DModel.surfaceVisible()
                    text: "Display surface"
                    onCheckedChanged: Settings3DController.changeSurfaceVisibility(checked)
                }

                CCheck {
                    id: displayGridCheckBox
                    Layout.fillWidth: true
                    checked: Scene3DModel.surfaceGridVisible()
                    text: "Display grid"
                    onCheckedChanged: Settings3DController.changeSurfaceGridVisibility(checked)
                }
            }

            ParamSetup {
                paramName: "Calculation method:"
                CCombo  {
                    id: calcMethodCCombo
                    Layout.fillWidth: true
                    model: ["TIN"]
                    enabled:Scene3DModel.triangulationAvailable()
                    currentIndex: 0
                    onCurrentTextChanged: Settings3DController.changeCalculationMethod(currentText)

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
                }
            }

            ParamSetup {
                paramName: "Grid type:"
                visible: smoothMethodCCombo.currentText != "None"

                CCombo  {
                    id: gridTypeCombo
                    Layout.fillWidth: true
                    model: ["Quad", "Triangle"]
                    enabled:Scene3DModel.triangulationAvailable()
                    currentIndex: 0
                    onCurrentTextChanged: Settings3DController.changeGridType(currentText)

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
                }
            }

        }

        ParamGroup {
            groupName: "Contour"
            Layout.margins: 10
            spacing: 24

            RowLayout {
                    width: parent.width

                CCheck {
                    id: displayContourCheckBox
                    Layout.fillWidth: true
                    checked: Scene3DModel.contourVisibility()
                    text: "Display contour"
                    onCheckedChanged: Settings3DController.changeContourVisibility(checked)
                }

                CCheck{
                    id: displayControlPointsCheckBox
                    Layout.fillWidth: true
                    checked: Scene3DModel.contourKeyPointsVisibility()
                    text: "Display key points"
                    onCheckedChanged: Settings3DController.changeContourKeyPointsVisibility(checked)
                }
            }

            RowLayout {
                width: parent.width

                CText{
                    text: "Line color: "
                    Layout.alignment: Qt.AlignLeft
                    Layout.fillWidth: true
                }

                CButton {
                    id: contourColorButton
                    onClicked: colorDialog.open()
                    text:""
                    Layout.maximumWidth: 100
                    Layout.minimumWidth: 100
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignRight

                    background: Rectangle {
                        color: contourColor

                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true

                CText{
                    text: "Key points color: "
                    Layout.alignment: Qt.AlignLeft
                    Layout.fillWidth: true
                }

                CButton {
                    id: contourKeyPointsColorButton
                    onClicked: contourKeyPointsColorDialog.open()
                    text:""
                    Layout.maximumWidth: 100
                    Layout.minimumWidth: 100
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignRight


                    background: Rectangle {
                        color: contourKeyPointsColor
                    }
                }
            }

            ParamSetup {
                paramName: "Line width: "

                Slider {
                    id: lineWidthSlider
                    Layout.fillWidth: true
                    stepSize: 1.0
                    from: 1.0
                    value: 6.0
                    to: 10.0
                    onValueChanged: Settings3DController.changeContourLineWidth(value)
                }
            }

            ColorDialog {
                id: colorDialog
                title: "Contour color picker"
                onAccepted: Settings3DController.changeContourColor(currentColor)
            }

            ColorDialog {
                id: contourKeyPointsColorDialog
                title: "Contour key points color picker"
                onAccepted: Settings3DController.changeContourKeyPointsColor(currentColor)
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

            background: Rectangle {
                color: {
                    !updateSurfaceButton.enabled ? theme.disabledBackColor :
                     updateSurfaceButton.hovered ? theme.hoveredBackColor  :
                                                   theme.controlBackColor
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
