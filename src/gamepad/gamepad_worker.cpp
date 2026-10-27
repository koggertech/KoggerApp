#include "gamepad_worker.h"

#include <algorithm>

#ifdef KOGGER_SDL3
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#endif

#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif

namespace {
constexpr int kPollIntervalMs = 16;

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
        setAndroidEventRouting(true);
    }

    emit errorChanged(QString());
    openFirstPad();
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

#ifdef KOGGER_SDL3
    if (subsystemUp_) {
        setAndroidEventRouting(false);
        SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
        subsystemUp_ = false;
    }
#endif
}

void GamepadWorker::poll()
{
#ifdef KOGGER_SDL3
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_GAMEPAD_ADDED && !pad_) {
            openFirstPad();
        }
        else if (event.type == SDL_EVENT_GAMEPAD_REMOVED && pad_ && event.gdevice.which == SDL_GetGamepadID(pad_)) {
            closePad();
            openFirstPad();
        }
    }

    readState(false);
#endif
}

void GamepadWorker::openFirstPad()
{
#ifdef KOGGER_SDL3
    if (pad_) {
        return;
    }

    int count = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&count);
    for (int i = 0; ids && i < count && !pad_; ++i) {
        pad_ = SDL_OpenGamepad(ids[i]);
    }
    SDL_free(ids);

    if (!pad_) {
        return;
    }

    const char* name = SDL_GetGamepadName(pad_);
    const char* type = SDL_GetGamepadStringForType(SDL_GetGamepadType(pad_));
    int available = 0;
    for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; ++b) {
        if (SDL_GamepadHasButton(pad_, static_cast<SDL_GamepadButton>(b))) {
            available |= 1 << b;
        }
    }

    emit connectionChanged(true,
                           name ? QString::fromUtf8(name) : QString(),
                           type ? QString::fromUtf8(type) : QString(),
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
    emit connectionChanged(false, QString(), QString(), 0);
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
            const qreal raw = SDL_GetGamepadAxis(pad_, static_cast<SDL_GamepadAxis>(a)) / 32767.0;
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
