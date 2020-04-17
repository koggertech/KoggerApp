import QtQuick 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4

//import SerialList 1.0

Item {
    id: menu

    property var lastItem: null

    function itemChangeActive(currentItem) {
        if(currentItem !== null) {
            currentItem.active = !(currentItem.active)
        }

        if(lastItem !== null && lastItem !== currentItem) {
           lastItem.active = false
        }

        lastItem = currentItem
    }

    RowLayout {
        id: menuLayout

        MenuButton {
            id: menuConnection
            text: "C"

            onPressed: {
                itemChangeActive(menuConnection)
            }

            ConnectionViewer {
            }
        }

//        MenuButton {
//            id: menuSettings
//            text: "S"

//            onPressed: {
//                itemChangeActive(menuSettings)
//            }

//            DeviceSettingsViewer {
//            }
//        }

//        MenuButton {
//            id: menuDipslay
//            text: "D"

//            onPressed: {
//                itemChangeActive(menuDipslay)
//            }
//        }
    }
}
