import QtQuick 2.15

QtObject {
    id: root

    readonly property var tracker: typeof inputDeviceTracker !== "undefined" ? inputDeviceTracker : null
    property string currentMode: tracker ? tracker.currentMode : "mouseKeyboard"
    property int suppressMouseUntilMs: 0

    readonly property bool touchMode: tracker ? tracker.touchMode : currentMode === "touchScreen"
    readonly property string displayLabel: tracker ? tracker.displayLabel : (touchMode ? "Touchscreen" : "Keyboard+Mouse")
    readonly property color displayColor: tracker ? tracker.displayColor : (touchMode ? "#16A34A" : "#2563EB")

    function markMouseKeyboardInput() {
        if (tracker && typeof tracker.markMouseKeyboardInput === "function") {
            tracker.markMouseKeyboardInput()
            return
        }

        var now = Date.now()
        if (now < suppressMouseUntilMs)
            return
        if (currentMode !== "mouseKeyboard")
            currentMode = "mouseKeyboard"
    }

    function markKeyboardInput() {
        if (tracker && typeof tracker.markKeyboardInput === "function") {
            tracker.markKeyboardInput()
            return
        }

        if (currentMode !== "mouseKeyboard")
            currentMode = "mouseKeyboard"
    }

    function markTouchInput() {
        if (tracker && typeof tracker.markTouchInput === "function") {
            tracker.markTouchInput()
            return
        }

        suppressMouseUntilMs = Date.now() + 450
        if (currentMode !== "touchScreen")
            currentMode = "touchScreen"
    }
}
