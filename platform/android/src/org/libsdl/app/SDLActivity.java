package org.libsdl.app;

import android.app.Activity;
import android.app.UiModeManager;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.os.Handler;
import android.os.Looper;
import android.util.DisplayMetrics;
import android.view.Surface;
import android.view.View;

/**
 * Not the original SDL 3.4.2 file: a replacement written for KoggerApp.
 * JNI stub for an app whose Activity is Qt's. libSDL3.so registers the natives
 * below at load time and SDLActivity.nativeSetupJNI() looks up every static
 * callback below by name and signature; a missing one leaves a pending
 * NoSuchMethodError. Device queries forward to the real context, window,
 * cursor, keyboard and dialog requests are declined.
 */
public class SDLActivity {

    public static native String nativeGetVersion();
    public static native void nativeSetupJNI();
    public static native void nativeInitMainThread();
    public static native void nativeCleanupMainThread();
    public static native int nativeRunMain(String library, String function, Object arguments);
    public static native void onNativeDropFile(String filename);
    public static native void nativeSetScreenResolution(int surfaceWidth, int surfaceHeight, int deviceWidth, int deviceHeight, float density, float rate);
    public static native void onNativeResize();
    public static native void onNativeSurfaceCreated();
    public static native void onNativeSurfaceChanged();
    public static native void onNativeSurfaceDestroyed();
    public static native void onNativeScreenKeyboardShown();
    public static native void onNativeScreenKeyboardHidden();
    public static native void onNativeKeyDown(int keycode);
    public static native void onNativeKeyUp(int keycode);
    public static native boolean onNativeSoftReturnKey();
    public static native void onNativeKeyboardFocusLost();
    public static native void onNativeTouch(int touchDevId, int pointerFingerId, int action, float x, float y, float p);
    public static native void onNativePinchStart();
    public static native void onNativePinchUpdate(float scale);
    public static native void onNativePinchEnd();
    public static native void onNativeMouse(int button, int action, float x, float y, boolean relative);
    public static native void onNativePen(int penId, int device_type, int button, int action, float x, float y, float p);
    public static native void onNativeAccel(float x, float y, float z);
    public static native void onNativeClipboardChanged();
    public static native void nativeLowMemory();
    public static native void onNativeLocaleChanged();
    public static native void onNativeDarkModeChanged(boolean enabled);
    public static native void nativeSendQuit();
    public static native void nativeQuit();
    public static native void nativePause();
    public static native void nativeResume();
    public static native void nativeFocusChanged(boolean hasFocus);
    public static native String nativeGetHint(String name);
    public static native boolean nativeGetHintBoolean(String name, boolean default_value);
    public static native void nativeSetenv(String name, String value);
    public static native void nativeSetNaturalOrientation(int orientation);
    public static native void onNativeRotationChanged(int rotation);
    public static native void onNativeInsetsChanged(int left, int right, int top, int bottom);
    public static native void nativeAddTouch(int touchId, String name);
    public static native void nativePermissionResult(int requestCode, boolean result);
    public static native boolean nativeAllowRecreateActivity();
    public static native int nativeCheckSDLThreadCounter();
    public static native void onNativeFileDialog(int requestCode, String[] filelist, int filter);

    public static Activity getContext() {
        return SDL.getContext();
    }

    public static View getContentView() {
        final Activity activity = getContext();
        return activity != null ? activity.getWindow().getDecorView() : null;
    }

    public static boolean isAndroidTV() {
        final Activity activity = getContext();
        if (activity == null) {
            return false;
        }
        final UiModeManager uiModeManager = (UiModeManager) activity.getSystemService(Context.UI_MODE_SERVICE);
        return uiModeManager != null && uiModeManager.getCurrentModeType() == Configuration.UI_MODE_TYPE_TELEVISION;
    }

    public static boolean isTablet() {
        final Activity activity = getContext();
        if (activity == null) {
            return false;
        }
        final DisplayMetrics metrics = activity.getResources().getDisplayMetrics();
        final double widthInches = metrics.widthPixels / (double) metrics.xdpi;
        final double heightInches = metrics.heightPixels / (double) metrics.ydpi;
        return Math.sqrt(widthInches * widthInches + heightInches * heightInches) >= 7.0;
    }

    public static boolean isChromebook() {
        final Activity activity = getContext();
        if (activity == null) {
            return false;
        }
        final PackageManager pm = activity.getPackageManager();
        return pm.hasSystemFeature("org.chromium.arc") || pm.hasSystemFeature("org.chromium.arc.device_management");
    }

    public static boolean isDeXMode() {
        final Activity activity = getContext();
        if (activity == null) {
            return false;
        }
        try {
            final Configuration config = activity.getResources().getConfiguration();
            final Class<?> configClass = config.getClass();
            return configClass.getField("SEM_DESKTOP_MODE_ENABLED").getInt(configClass)
                    == configClass.getField("semDesktopModeEnabled").getInt(config);
        } catch (Exception ignored) {
            return false;
        }
    }

    public static void requestPermission(String permission, int requestCode) {
        final Activity activity = getContext();
        final boolean granted = activity != null
                && activity.checkSelfPermission(permission) == PackageManager.PERMISSION_GRANTED;
        new Handler(Looper.getMainLooper()).post(() -> nativePermissionResult(requestCode, granted));
    }

    public static boolean getManifestEnvironmentVariables() {
        return false;
    }

    public static String getPreferredLocales() {
        return "";
    }

    public static Surface getNativeSurface() {
        return null;
    }

    public static void initTouch() {
    }

    public static void manualBackButton() {
    }

    public static void minimizeWindow() {
    }

    public static boolean shouldMinimizeOnFocusLoss() {
        return false;
    }

    public static boolean sendMessage(int command, int param) {
        return false;
    }

    public static boolean setActivityTitle(String title) {
        return false;
    }

    public static void setWindowStyle(boolean fullscreen) {
    }

    public static void setOrientation(int w, int h, boolean resizable, String hint) {
    }

    public static boolean showTextInput(int input_type, int x, int y, int w, int h) {
        return false;
    }

    public static boolean showToast(String message, int duration, int gravity, int xOffset, int yOffset) {
        return false;
    }

    public static boolean openURL(String url) {
        return false;
    }

    public static boolean clipboardHasText() {
        return false;
    }

    public static String clipboardGetText() {
        return "";
    }

    public static void clipboardSetText(String string) {
    }

    public static int createCustomCursor(int[] colors, int width, int height, int hotSpotX, int hotSpotY) {
        return 0;
    }

    public static void destroyCustomCursor(int cursorID) {
    }

    public static boolean setCustomCursor(int cursorID) {
        return false;
    }

    public static boolean setSystemCursor(int cursorID) {
        return false;
    }

    public static boolean supportsRelativeMouse() {
        return false;
    }

    public static boolean setRelativeMouseEnabled(boolean enabled) {
        return false;
    }

    public static int openFileDescriptor(String uri, String mode) {
        return -1;
    }

    public static boolean showFileDialog(String[] filters, boolean allowMultiple, boolean forWrite, int requestCode) {
        return false;
    }
}
