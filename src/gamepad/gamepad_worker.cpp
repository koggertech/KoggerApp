#include "gamepad_worker.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>

#ifdef KOGGER_SDL3
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#endif

#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif

namespace {
constexpr int kPollIntervalMs = 16;
#ifdef KOGGER_SDL3
constexpr qreal kAxisScale = 32767.0;
constexpr qreal kTriggerSpan = 65535.0;
constexpr int kTriggerBase = -32768;
constexpr char kVirtualPadName[] = "KoggerApp virtual gamepad";
constexpr Uint16 kVendorValve = 0x28DE;
constexpr qreal kRumbleSpan = 65535.0;
constexpr Uint32 kRumbleHoldMs = 250;

QString padFamily(SDL_Gamepad* pad)
{
    switch (SDL_GetGamepadType(pad)) {
    case SDL_GAMEPAD_TYPE_XBOX360:
    case SDL_GAMEPAD_TYPE_XBOXONE:
        return QStringLiteral("xbox");
    case SDL_GAMEPAD_TYPE_PS4:
        return QStringLiteral("ps4");
    case SDL_GAMEPAD_TYPE_PS5:
        return QStringLiteral("ps5");
    default:
        break;
    }
    return SDL_GetGamepadVendor(pad) == kVendorValve ? QStringLiteral("steam") : QStringLiteral("generic");
}

QString faceLabel(SDL_Gamepad* pad, SDL_GamepadButton button)
{
    switch (SDL_GetGamepadButtonLabel(pad, button)) {
    case SDL_GAMEPAD_BUTTON_LABEL_A:
        return QStringLiteral("A");
    case SDL_GAMEPAD_BUTTON_LABEL_B:
        return QStringLiteral("B");
    case SDL_GAMEPAD_BUTTON_LABEL_X:
        return QStringLiteral("X");
    case SDL_GAMEPAD_BUTTON_LABEL_Y:
        return QStringLiteral("Y");
    case SDL_GAMEPAD_BUTTON_LABEL_CROSS:
        return QStringLiteral("cross");
    case SDL_GAMEPAD_BUTTON_LABEL_CIRCLE:
        return QStringLiteral("circle");
    case SDL_GAMEPAD_BUTTON_LABEL_SQUARE:
        return QStringLiteral("square");
    case SDL_GAMEPAD_BUTTON_LABEL_TRIANGLE:
        return QStringLiteral("triangle");
    default:
        return QString();
    }
}
#endif

void setAndroidEventRouting(bool enabled)
{
#ifdef Q_OS_ANDROID
    QJniObject::callStaticMethod<void>("org/kogger/koggerapp/KoggerGamepadInput", "setRouting", "(Z)V",
                                       static_cast<jboolean>(enabled));
#else
    Q_UNUSED(enabled);
#endif
}
}

GamepadWorker::GamepadWorker(QObject* parent)
    : QObject(parent),
      timer_(this)
{
    timer_.setInterval(kPollIntervalMs);
    timer_.setTimerType(Qt::PreciseTimer);
    connect(&timer_, &QTimer::timeout, this, &GamepadWorker::poll);
}

GamepadWorker::~GamepadWorker()
{
    stop();
#ifdef KOGGER_SDL3
    if (sdlUsed_) {
        SDL_Quit();
    }
#endif
}

void GamepadWorker::start()
{
#ifdef KOGGER_SDL3
    if (!subsystemUp_) {
        SDL_SetMainReady();
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
        if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
            emit errorChanged(QString::fromUtf8(SDL_GetError()));
            return;
        }
        subsystemUp_ = true;
        sdlUsed_ = true;
        applyRouting();
        emit readyChanged(true);
    }

    emit errorChanged(QString());
    selectLatestPad();
    readState(true);
    timer_.start();
#else
    emit errorChanged(QStringLiteral("SDL3 is not linked into this build"));
#endif
}

void GamepadWorker::stop()
{
    timer_.stop();
    closePad();
    detachVirtualPad();

#ifdef KOGGER_SDL3
    if (subsystemUp_) {
        setAndroidEventRouting(false);
        SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
        subsystemUp_ = false;
        emit readyChanged(false);
    }
#endif
}

void GamepadWorker::poll()
{
#ifdef KOGGER_SDL3
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_GAMEPAD_ADDED && !continuous_) {
            selectLatestPad();
        }
        else if (event.type == SDL_EVENT_GAMEPAD_REMOVED && pad_ && event.gdevice.which == SDL_GetGamepadID(pad_)) {
            closePad();
            if (!continuous_) {
                selectLatestPad();
            }
        }
    }

    readState(continuous_);
#endif
}

void GamepadWorker::setRouting(bool routing)
{
    routing_ = routing;
    applyRouting();
}

void GamepadWorker::setContinuous(bool continuous)
{
    continuous_ = continuous;
#ifdef KOGGER_SDL3
    if (!continuous_ && subsystemUp_) {
        selectLatestPad();
    }
#endif
}

void GamepadWorker::applyRouting()
{
    if (subsystemUp_) {
        setAndroidEventRouting(routing_);
    }
}

void GamepadWorker::attachVirtualPad(int vendorId, int productId)
{
#ifdef KOGGER_SDL3
    if (!subsystemUp_ || virtualPad_) {
        return;
    }

    SDL_VirtualJoystickDesc desc;
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.vendor_id = Uint16(vendorId);
    desc.product_id = Uint16(productId);
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.axis_mask = (1u << SDL_GAMEPAD_AXIS_COUNT) - 1u;
    desc.button_mask = (1u << SDL_GAMEPAD_BUTTON_COUNT) - 1u;
    desc.name = kVirtualPadName;

    const SDL_JoystickID id = SDL_AttachVirtualJoystick(&desc);
    if (id == 0) {
        emit errorChanged(QString::fromUtf8(SDL_GetError()));
        return;
    }
    virtualPad_ = SDL_OpenJoystick(id);
    if (!virtualPad_) {
        emit errorChanged(QString::fromUtf8(SDL_GetError()));
        SDL_DetachVirtualJoystick(id);
        return;
    }
    setVirtualInput(0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
#else
    Q_UNUSED(vendorId);
    Q_UNUSED(productId);
#endif
}

void GamepadWorker::detachVirtualPad()
{
#ifdef KOGGER_SDL3
    if (!virtualPad_) {
        return;
    }

    const SDL_JoystickID id = SDL_GetJoystickID(virtualPad_);
    const bool wasOpen = pad_ && SDL_GetGamepadID(pad_) == id;
    if (wasOpen) {
        closePad();
    }
    SDL_CloseJoystick(virtualPad_);
    virtualPad_ = nullptr;
    SDL_DetachVirtualJoystick(id);
    if (wasOpen && subsystemUp_ && !continuous_) {
        selectLatestPad();
    }
#endif
}

void GamepadWorker::setVirtualInput(int buttons, qreal leftX, qreal leftY, qreal rightX, qreal rightY, qreal leftTrigger, qreal rightTrigger)
{
#ifdef KOGGER_SDL3
    if (!virtualPad_) {
        return;
    }

    for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; ++b) {
        SDL_SetJoystickVirtualButton(virtualPad_, b, (buttons & (1 << b)) != 0);
    }

    const std::array<qreal, 4> sticks{ leftX, leftY, rightX, rightY };
    for (int a = 0; a < int(sticks.size()); ++a) {
        SDL_SetJoystickVirtualAxis(virtualPad_, a, Sint16(std::lround(std::clamp(sticks[a], -1.0, 1.0) * kAxisScale)));
    }
    const std::array<qreal, 2> triggers{ leftTrigger, rightTrigger };
    for (int t = 0; t < int(triggers.size()); ++t) {
        const long value = kTriggerBase + std::lround(std::clamp(triggers[t], 0.0, 1.0) * kTriggerSpan);
        SDL_SetJoystickVirtualAxis(virtualPad_, SDL_GAMEPAD_AXIS_LEFT_TRIGGER + t, Sint16(value));
    }
#else
    Q_UNUSED(buttons);
    Q_UNUSED(leftX);
    Q_UNUSED(leftY);
    Q_UNUSED(rightX);
    Q_UNUSED(rightY);
    Q_UNUSED(leftTrigger);
    Q_UNUSED(rightTrigger);
#endif
}

void GamepadWorker::setRumble(qreal low, qreal high)
{
#ifdef KOGGER_SDL3
    if (!pad_) {
        return;
    }

    const Uint16 lowLevel = Uint16(std::lround(std::clamp(low, 0.0, 1.0) * kRumbleSpan));
    const Uint16 highLevel = Uint16(std::lround(std::clamp(high, 0.0, 1.0) * kRumbleSpan));
    SDL_RumbleGamepad(pad_, lowLevel, highLevel, (lowLevel != 0 || highLevel != 0) ? kRumbleHoldMs : 0);
#else
    Q_UNUSED(low);
    Q_UNUSED(high);
#endif
}

void GamepadWorker::selectLatestPad()
{
#ifdef KOGGER_SDL3
    int count = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&count);
    std::vector<SDL_JoystickID> order(ids, ids + (ids ? count : 0));
    SDL_free(ids);
    std::sort(order.begin(), order.end(), std::greater<>());

    const SDL_JoystickID current = pad_ ? SDL_GetGamepadID(pad_) : 0;
    for (SDL_JoystickID id : order) {
        if (id == current) {
            return;
        }
        if (SDL_Gamepad* next = SDL_OpenGamepad(id)) {
            if (pad_) {
                SDL_CloseGamepad(pad_);
            }
            pad_ = next;
            emitConnection();
            readState(true);
            return;
        }
    }
#endif
}

void GamepadWorker::emitConnection()
{
#ifdef KOGGER_SDL3
    const char* name = SDL_GetGamepadName(pad_);
    const char* type = SDL_GetGamepadStringForType(SDL_GetGamepadType(pad_));
    int available = 0;
    for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; ++b) {
        if (SDL_GamepadHasButton(pad_, static_cast<SDL_GamepadButton>(b))) {
            available |= 1 << b;
        }
    }
    const QStringList faces{ faceLabel(pad_, SDL_GAMEPAD_BUTTON_SOUTH), faceLabel(pad_, SDL_GAMEPAD_BUTTON_EAST),
                             faceLabel(pad_, SDL_GAMEPAD_BUTTON_WEST), faceLabel(pad_, SDL_GAMEPAD_BUTTON_NORTH) };

    emit connectionChanged(true,
                           name ? QString::fromUtf8(name) : QString(),
                           type ? QString::fromUtf8(type) : QString(),
                           padFamily(pad_),
                           faces,
                           available);
#endif
}

void GamepadWorker::closePad()
{
#ifdef KOGGER_SDL3
    if (!pad_) {
        return;
    }

    SDL_CloseGamepad(pad_);
    pad_ = nullptr;
    emit connectionChanged(false, QString(), QString(), QString(), QStringList(), 0);
    readState(false);
#endif
}

void GamepadWorker::readState(bool force)
{
    int buttons = 0;
    std::array<qreal, 6> axes{};

#ifdef KOGGER_SDL3
    if (pad_) {
        for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; ++b) {
            if (SDL_GetGamepadButton(pad_, static_cast<SDL_GamepadButton>(b))) {
                buttons |= 1 << b;
            }
        }
        for (int a = 0; a < SDL_GAMEPAD_AXIS_COUNT; ++a) {
            const qreal raw = SDL_GetGamepadAxis(pad_, static_cast<SDL_GamepadAxis>(a)) / kAxisScale;
            axes[a] = std::clamp(raw, -1.0, 1.0);
        }
    }
#endif

    if (!force && buttons == buttons_ && axes == axes_) {
        return;
    }

    buttons_ = buttons;
    axes_ = axes;
    emit stateChanged(buttons_, axes_[0], axes_[1], axes_[2], axes_[3], axes_[4], axes_[5]);
}
