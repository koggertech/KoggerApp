import QtQuick 2.15
import kqml_types 1.0

Item {
    id: editPage

    required property var store

    width: parent ? parent.width : implicitWidth
    implicitHeight: stepLoader.item ? stepLoader.item.implicitHeight : 0

    // 0 = which kind, 1 = grid size, 2 = place fields, 3 = the acoustic-nodes panel, 4 = the
    // servo panel. Steps 1 and 2 are the grid wizard; 3 and 4 are the whole of the other two.
    // `place` stays the fallback so an out-of-range step lands somewhere usable rather than
    // blank.
    readonly property int _step: editPage.store ? editPage.store.widgetEditStep : 2

    Loader {
        id: stepLoader
        width: editPage.width
        sourceComponent: editPage._step === 0 ? kindComp
                       : editPage._step === 1 ? sizeComp
                       : editPage._step === 3 ? usblComp
                       : editPage._step === 4 ? servoComp
                                              : placeComp
    }

    Component { id: kindComp;  WidgetKindStep  { store: editPage.store } }
    Component { id: sizeComp;  WidgetSizeStep  { store: editPage.store } }
    Component { id: placeComp; WidgetPlaceStep { store: editPage.store } }
    Component { id: usblComp;  WidgetUsblStep  { store: editPage.store } }
    Component { id: servoComp; WidgetServoStep { store: editPage.store } }
}
