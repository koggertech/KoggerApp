import QtQuick 2.12
import QtQuick.Controls 2.12

Text {
    property bool small: false
    font: small ? theme.textFontS : theme.textFont
    color: theme.textColor
}
