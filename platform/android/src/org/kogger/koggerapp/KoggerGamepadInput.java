package org.kogger.koggerapp;

import android.app.Activity;
import android.util.Log;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;

import org.libsdl.app.HIDDeviceManager;
import org.libsdl.app.SDL;
import org.libsdl.app.SDLControllerManager;

/**
 * Game controller input for KoggerActivity, backed by SDL3. Without
 * libSDL3.so in the APK attach() logs once and the rest does nothing.
 * Controller events reach SDL only while the native side has SDL's gamepad
 * subsystem up (setRouting); otherwise they stay with Qt.
 */
public final class KoggerGamepadInput {
    private static final String TAG = KoggerGamepadInput.class.getSimpleName();

    private static boolean m_sdlLoaded = false;
    private static volatile boolean m_routing = false;
    private static HIDDeviceManager m_hid = null;

    private KoggerGamepadInput() {
    }

    /**
     * Loads SDL, binds its JNI callbacks to the activity and starts HID discovery.
     *
     * @param activity The activity SDL uses for context and system services.
     */
    public static void attach(Activity activity) {
        if (!m_sdlLoaded) {
            try {
                System.loadLibrary("SDL3");
            } catch (final UnsatisfiedLinkError e) {
                Log.w(TAG, "libSDL3.so is not packaged, gamepad input disabled");
                return;
            }

            try {
                SDL.setupJNI();
            } catch (final UnsatisfiedLinkError e) {
                Log.e(TAG, "SDL Java glue does not match libSDL3.so, gamepad input disabled", e);
                return;
            }
            m_sdlLoaded = true;
        }

        try {
            SDL.initialize();
            SDL.setContext(activity);

            if (m_hid == null) {
                m_hid = HIDDeviceManager.acquire(activity);
            }

            Log.i(TAG, "Gamepad input attached");
        } catch (final Exception e) {
            Log.e(TAG, "Gamepad input attach failed", e);
        }
    }

    /**
     * Called from the native gamepad thread around SDL_InitSubSystem/SDL_QuitSubSystem.
     *
     * @param enabled true while SDL's gamepad subsystem is up.
     */
    public static void setRouting(boolean enabled) {
        m_routing = enabled;
    }

    /**
     * Stops HID discovery and forgets the activity.
     */
    public static void detach() {
        if (m_hid != null) {
            HIDDeviceManager.release(m_hid);
            m_hid = null;
        }

        if (m_sdlLoaded) {
            SDL.setContext(null);
        }
    }

    /**
     * Freezes or resumes HID polling with the activity's visibility.
     *
     * @param inForeground false while the activity is paused.
     */
    public static void setForeground(boolean inForeground) {
        if (m_hid != null) {
            m_hid.setFrozen(!inForeground);
        }
    }

    /**
     * Passes stick and hat motion from a controller to SDL.
     *
     * @param event Generic motion event seen by the activity.
     * @return true when the event belongs to SDL and must not reach Qt.
     */
    public static boolean takeMotion(MotionEvent event) {
        if (!m_sdlLoaded || !m_routing || !fromController(event.getSource())) {
            return false;
        }

        return SDLControllerManager.handleJoystickMotionEvent(event);
    }

    /**
     * Passes controller buttons to SDL; keyboard and system keys stay with Qt.
     *
     * @param event Key event seen by the activity.
     * @return true when the event belongs to SDL and must not reach Qt.
     */
    public static boolean takeKey(KeyEvent event) {
        if (!m_sdlLoaded || !m_routing || !fromController(event.getSource())) {
            return false;
        }

        switch (event.getAction()) {
            case KeyEvent.ACTION_DOWN:
                return SDLControllerManager.onNativePadDown(event.getDeviceId(), event.getKeyCode());
            case KeyEvent.ACTION_UP:
                return SDLControllerManager.onNativePadUp(event.getDeviceId(), event.getKeyCode());
            default:
                return false;
        }
    }

    private static boolean fromController(int source) {
        return (source & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD
                || (source & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK;
    }
}
