import QtQuick 2.15
import QtQuick.Controls 2.15

Text {
    property bool small: false
    font: small ? theme.textFontS : theme.textFont
    color: theme.textColor
}
