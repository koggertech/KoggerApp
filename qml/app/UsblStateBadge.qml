import QtQuick 2.15
import kqml_types 1.0

// What the BEACON did -- the outcome of the last interrogation -- and how old the readings that
// came out of it are. Takes the codes UsblNodeLogic.nodeReplyCode and stepCode return.
//
// This is the only place the three words exist. Both surfaces that draw a node state go through
// this file, so neither can grow a second vocabulary; test_usbl_node_logic.mjs asserts that.
//
// TWO FACTS, ONE CHIP, AND THEY MUST NOT BE CONFUSED. The verdict fills the chip when it is a
// miss. The age never does -- it warns in its own INK, so a node that is answering but has not
// been asked lately reads "tick, REPLIED, amber 34 s" rather than looking like a failure.
// See docs/KoggerApp-Docs/usbl-node-row.md.
Rectangle {
    id: badge

    property string code: "none"            // "replied" | "stale" | "none"
    property real chipHeight: Tokens.chipH
    // The word is the primary thing in here; the age is a note beside it. Two sizes, so the
    // hierarchy is in the type rather than in the colour, which is already carrying the state.
    property real fontPixelSize: Tokens.fontSm
    property real ageFontPixelSize: Tokens.fontXxs
    // Off dims the row, not the chip -- except in the panel, which has no switch to say so.
    property bool dimmed: false

    // The age, already formatted by the caller (the two surfaces spell it differently), and
    // whether it has passed AGE_WARN_MS. Empty draws an empty slot, not a narrower chip: a node
    // nobody has heard from still has to line up with the rows around it.
    property string age: ""
    property bool aged: false
    // Whether this node's misses have gone past LOST_MISSES. Comes from the reducer via the
    // caller -- the badge is not in a position to count interrogations.
    property bool lost: false
    // What the age SLOT is sized to. The age is a number that ticks, so the slot is fixed and
    // the value is drawn into it -- a chip that resizes every second is the jitter the fixed
    // width exists to prevent. Empty means no ages anywhere, so no slot at all.
    property string ageSample: ""

    readonly property bool _ok: badge.code === "replied"

    // THREE SEVERITIES OUT OF TWO CODES. One dropped interrogation on a moving vessel is routine
    // and clears itself; a node that has refused LOST_MISSES in a row is not the same event, and
    // an amber that means both means neither.
    //
    // The escalation is COUNTED BY THE REDUCER, not derived here from the age. It was derived
    // from the age once and it could not work: Dataset caches a solution per address on arrival
    // whether or not a window was open to attribute it to, so a reply landing after its window
    // closed still resets the age. A node answering just too late to be counted missed every
    // interrogation with its age resetting every time -- permanently MISSED, never LOST.
    readonly property bool _lost: badge.code === "stale" && badge.lost
    readonly property bool _bad:  badge.code === "stale" && !badge.lost

    readonly property var _text: ({
        "replied": qsTr("REPLIED"),
        "stale":   qsTr("MISSED"),
        "lost":    qsTr("LOST"),
        "none":    qsTr("NEVER")
    })
    readonly property string _key: _lost ? "lost" : badge.code

    // Ink for a FILLED chip. The amber pair is a bright border colour on a very dark fill in the
    // dark themes, which glares rather than reads; stepping the ink down closes the gap without
    // touching the token every other link indicator shares.
    readonly property color _amberInk: AppPalette.isDark
                                       ? Qt.darker(AppPalette.linkIdleBorder, 1.22)
                                       : AppPalette.linkIdleText

    // What the mark is drawn in -- and the repaint trigger, because a theme switch changes it
    // without touching anything else the Canvas is bound to and a Canvas does not track that.
    readonly property color _ink: _lost ? AppPalette.dangerText
                                : _bad  ? _amberInk
                                : _ok   ? AppPalette.linkOkBorder : AppPalette.textMuted

    readonly property real _markSize: Math.round(chipHeight * 0.58)
    readonly property real _gap:      Math.round(chipHeight * 0.21)
    readonly property real _padH:     Math.round(chipHeight * 0.29)

    // BOTH SLOTS ARE MEASURED, NOT PINNED. The word slot is the widest of the three as this
    // locale renders them, so a longer translation widens the chip instead of clipping inside
    // it; the age slot is what the caller says the widest age on screen is. Every state is then
    // the same width, and -- because the word is drawn into a fixed slot rather than centred --
    // the mark beside it does not shift as the verdict changes.
    readonly property real _wordW: Math.max(_mReplied.width, _mMissed.width,
                                            _mLost.width, _mNever.width)
    readonly property real _ageW: badge.ageSample.length ? _mAge.width : 0

    implicitWidth: Math.ceil(_padH * 2 + _markSize + _gap + _wordW
                             + (_ageW > 0 ? _gap + _ageW : 0))
    implicitHeight: chipHeight
    width: implicitWidth
    height: implicitHeight
    radius: Tokens.radiusSm
    opacity: badge.dimmed ? 0.62 : 1.0

    color: _lost ? AppPalette.dangerBg : _bad ? AppPalette.linkIdleBg : "transparent"
    border.width: Math.max(1, Math.round(1 * AppPalette.scale))
    border.color: _lost ? AppPalette.dangerBorder
                : _bad  ? AppPalette.linkIdleBorder : AppPalette.border
    Behavior on color { ColorAnimation { duration: 200; easing.type: Easing.OutCubic } }

    TextMetrics { id: _mReplied; font: _label.font; text: badge._text["replied"] }
    TextMetrics { id: _mMissed;  font: _label.font; text: badge._text["stale"] }
    TextMetrics { id: _mLost;    font: _label.font; text: badge._text["lost"] }
    TextMetrics { id: _mNever;   font: _label.font; text: badge._text["none"] }
    TextMetrics { id: _mAge;     font: _ageLabel.font; text: badge.ageSample }

    Row {
        anchors.left: parent.left
        anchors.leftMargin: badge._padH
        anchors.verticalCenter: parent.verticalCenter
        spacing: badge._gap

        Canvas {
            id: mark
            width: badge._markSize
            height: badge._markSize
            anchors.verticalCenter: parent.verticalCenter
            antialiasing: true

            onPaint: {
                var ctx = getContext("2d")
                if (!ctx)
                    return
                ctx.clearRect(0, 0, width, height)
                var s = width
                ctx.lineCap = "round"
                ctx.lineJoin = "round"

                if (badge.code === "replied") {
                    ctx.strokeStyle = badge._ink
                    ctx.lineWidth = Math.max(1.4, s * 0.157)
                    ctx.beginPath()
                    ctx.moveTo(s * 0.186, s * 0.529)
                    ctx.lineTo(s * 0.407, s * 0.757)
                    ctx.lineTo(s * 0.829, s * 0.250)
                    ctx.stroke()
                } else if (badge.code === "stale") {
                    ctx.strokeStyle = badge._ink
                    ctx.lineWidth = Math.max(1.4, s * 0.157)
                    ctx.beginPath()
                    ctx.moveTo(s * 0.229, s * 0.243)
                    ctx.lineTo(s * 0.771, s * 0.757)
                    ctx.moveTo(s * 0.771, s * 0.243)
                    ctx.lineTo(s * 0.229, s * 0.757)
                    ctx.stroke()
                } else {
                    // Never heard from: a broken line going nowhere. Not a fault, so it stays in
                    // the muted ink rather than taking a colour.
                    ctx.strokeStyle = AppPalette.textMuted
                    ctx.lineWidth = Math.max(1, s * 0.129)
                    ctx.globalAlpha = 0.8
                    var y = s * 0.5, seg = s * 0.143, step = s * 0.314
                    for (var x = s * 0.214; x < s * 0.8; x += step) {
                        ctx.beginPath()
                        ctx.moveTo(x, y)
                        ctx.lineTo(x + seg, y)
                        ctx.stroke()
                    }
                }
            }
        }

        Text {
            id: _label
            width: badge._wordW
            anchors.verticalCenter: parent.verticalCenter
            text: badge._text[badge._key] !== undefined ? badge._text[badge._key] : badge.code
            color: badge._lost ? AppPalette.dangerText
                 : badge._bad  ? badge._amberInk
                 : badge._ok   ? AppPalette.text : AppPalette.textMuted
            font.pixelSize: badge.fontPixelSize
            font.bold: true
            // Caps at a size down with tracking back in: the treatment that makes a word read as
            // a state label rather than as prose. The strings are uppercase at the source rather
            // than via font.capitalization, so what TextMetrics measures above is what is drawn.
            font.letterSpacing: badge.fontPixelSize * 0.05
        }

        // RIGHT-ALIGNED in its slot, so the ages form a column down the panel with the unit
        // aligned: a bigger number then extends leftward and the old one is found without
        // reading any of them.
        Text {
            id: _ageLabel
            visible: badge._ageW > 0
            width: badge._ageW
            horizontalAlignment: Text.AlignRight
            anchors.verticalCenter: parent.verticalCenter
            text: badge.age
            // On a filled chip the age takes the chip's own ink; on a neutral one it warns in
            // ink alone. That is what keeps "answering, but nobody has asked lately" from
            // looking like a failure -- an aged REPLIED row goes amber in the text and nowhere
            // else.
            color: badge._lost ? AppPalette.dangerText
                 : badge._bad  ? badge._amberInk
                 : badge.aged  ? AppPalette.linkIdleText : AppPalette.textMuted
            font.pixelSize: badge.ageFontPixelSize
            font.weight: Font.DemiBold
        }
    }

    onCodeChanged: mark.requestPaint()
    on_MarkSizeChanged: mark.requestPaint()
    on_InkChanged: mark.requestPaint()
    Component.onCompleted: mark.requestPaint()
}
