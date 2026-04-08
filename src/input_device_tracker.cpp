#include "input_device_tracker.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QEvent>
#include <QGuiApplication>

namespace
{
constexpr qint64 kTouchMouseSuppressMs = 450;
}

InputDeviceTracker::InputDeviceTracker(QObject* parent)
    : QObject(parent)
{
    if (auto* app = QGuiApplication::instance()) {
        app->installEventFilter(this);
    }
}

QString InputDeviceTracker::currentMode() const
{
    return m_mode == Mode::TouchScreen ? QStringLiteral("touchScreen")
                                       : QStringLiteral("mouseKeyboard");
}

bool InputDeviceTracker::touchMode() const
{
    return m_mode == Mode::TouchScreen;
}

QString InputDeviceTracker::displayLabel() const
{
    return touchMode() ? QStringLiteral("Touchscreen")
                       : QStringLiteral("Keyboard+Mouse");
}

QColor InputDeviceTracker::displayColor() const
{
    return touchMode() ? QColor(QStringLiteral("#16A34A"))
                       : QColor(QStringLiteral("#2563EB"));
}

void InputDeviceTracker::markMouseKeyboardInput()
{
    if (shouldSuppressMouseInput())
        return;

    setMode(Mode::MouseKeyboard);
}

void InputDeviceTracker::markKeyboardInput()
{
    setMode(Mode::MouseKeyboard);
}

void InputDeviceTracker::markTouchInput()
{
    m_suppressMouseUntilMs = nowMs() + kTouchMouseSuppressMs;
    setMode(Mode::TouchScreen);
}

bool InputDeviceTracker::eventFilter(QObject* watched, QEvent* event)
{
    Q_UNUSED(watched);

    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
        markTouchInput();
        break;

    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
    case QEvent::Wheel:
        markMouseKeyboardInput();
        break;

    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::ShortcutOverride:
        markKeyboardInput();
        break;

    default:
        break;
    }

    return QObject::eventFilter(watched, event);
}

void InputDeviceTracker::setMode(Mode mode)
{
    if (m_mode == mode)
        return;

    m_mode = mode;
    emit currentModeChanged();
}

qint64 InputDeviceTracker::nowMs() const
{
    return QDateTime::currentMSecsSinceEpoch();
}

bool InputDeviceTracker::shouldSuppressMouseInput() const
{
    return nowMs() < m_suppressMouseUntilMs;
}
