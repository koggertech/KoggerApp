import QtQuick 2.15
import kqml_types 1.0

Item {
    id: editPage

    required property var store

    width: parent ? parent.width : implicitWidth
    implicitHeight: stepLoader.item ? stepLoader.item.implicitHeight : 0

    Loader {
        id: stepLoader
        width: editPage.width
        sourceComponent: (editPage.store && editPage.store.widgetEditStep === 1) ? sizeComp : placeComp
    }

    Component { id: sizeComp;  WidgetSizeStep  { store: editPage.store } }
    Component { id: placeComp; WidgetPlaceStep { store: editPage.store } }
}
