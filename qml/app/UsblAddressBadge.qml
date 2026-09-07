import QtQuick 2.15
import kqml_types 1.0

// A node's ADDRESS, as one shape wherever a node is listed: the settings pane's rows and the
// on-scene panel's rows. The two used to say it differently -- "addr 2" in one, a bare "2" in
// the other -- and a bare number beside other numbers reads as a row index.
//
// IT CARRIES NO STATE, deliberately. A row already has an operation chip, a reply chip, a cmd
// chip and an age chip; tinting the address would be a fifth mark saying what the second
// already says, and the pane is built on one mark meaning one thing. An address does not stop
// being that node's address when the beacon goes quiet.
//
// The collapsed group header keeps its own dot-and-number chip and is state-coloured, which is
// right there: at that zoom it is the ONLY mark, so it has to carry the state as well.
//
// THE FILL IS THE ADDRESS'S OWN COLOUR, from UsblFieldLogic.ADDRESS_COLORS — one table, so the
// pane's row, the panel's row and (when it lands) the beacon's marker on the map are the same
// colour for the same node. That is what the colour is FOR: matching a row to a thing in the
// water at a glance.
//
// Identity, not state. A node's colour never changes; the chips beside it carry what it is
// doing. They cannot be confused because they are a different visual register — a pale tinted
// fill with a word, against a saturated solid with a digit.
//
// Two fills were tried before this and both failed for the same reason: `controlRaised` is a
// RAISED SURFACE token, so it landed within a shade of the pane it sits on, and the text colour
// contrasted but said nothing. A per-address colour contrasts AND identifies.
//
// Ink is picked by luminance, the same way AppPalette.accentText is, because the fill is fixed
// while the theme is not — the digit has to stay legible on amber and on slate alike.
Rectangle {
    id: badge

    property int address: 0
    // Tokens.chipH, not a literal: only `controlH` follows theme.controlHeight, so a hard-coded
    // size stays put while every control around it grows with the theme.
    property real diameter: Tokens.chipH

    readonly property color _fill: DataFieldCatalog.usblAddressColor(badge.address)
    readonly property color _ink: AppPalette.luminance(_fill) < 0.55 ? "#FFFFFF" : "#15202B"

    // A rounded square, not a disc: a circle with a number in it reads as an avatar or a list
    // index. The max() only stops a wider number from being clipped into an unreadable glyph.
    width: Math.max(diameter, _addrText.implicitWidth + Math.round(diameter * 0.45))
    height: diameter
    radius: Tokens.radiusSm
    color: _fill

    Text {
        id: _addrText
        anchors.centerIn: parent
        text: String(badge.address)
        color: badge._ink
        font.pixelSize: Math.max(8, Math.round(badge.diameter * 0.58))
        font.bold: true
    }
}
