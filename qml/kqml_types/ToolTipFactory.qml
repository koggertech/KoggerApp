pragma Singleton
import QtQuick 2.15

QtObject {
    readonly property Component toolTip: Component {
        KToolTip {}
    }
}
