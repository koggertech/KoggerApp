import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs

ColumnLayout {

    ParamSetup {
        paramName: qsTr("TIN edge length limit:")

        SpinBoxCustom {
            id: maxTriEdgeLengthSpinBox
            from: 0
            to: 100
            stepSize: 2
            value: 10
            onValueChanged: Settings3DController.changeMaxTriangulationLineLength(value)
        }

    }
}
