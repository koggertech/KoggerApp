import QtQuick 2.12
import QtQuick.Layouts 1.3

import KoggerCommon 1.0

Item {
    readonly property var controller : Scene3DControlMenuController

    id:         root
    objectName: "scene3DControlMenu"

    MenuBlockEx {
        id:           menuBlock
        anchors.fill: root

        KParamGroup {
            id: paramGroup
            anchors.left:    parent.left
            anchors.right:   parent.right
            anchors.top:     parent.top
            anchors.margins: 24
            spacing:         24
            groupName:       "Scene controls"

            KCheck {
                id:               showSceneBoundingBoxCheckBox
                objectName:       "showSceneBoundingBoxCheckBox"
                text:             qsTr("Show scene bounding box")
                Layout.fillWidth: true
                checked:          root.controller.sceneBoundingBoxVisible
                onCheckedChanged: root.controller.onShowSceneBoundingBoxCheckBoxChecked(checked)
            }

            KParamSetup {
                id: sceneVertialScale
                paramName: qsTr("Vertical scale: ")

                KSlider{
                    from: 1
                    to: 10
                    width: root.width / 2.0

                    value: root.controller.verticalScale
                    onValueChanged: root.controller.onVerticalScaleSliderValueChanged(value)
                }
            }
        }
    }
}
