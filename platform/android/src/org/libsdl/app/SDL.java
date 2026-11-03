package org.libsdl.app;

import android.app.Activity;

/**
 * Not the original SDL 3.4.2 file: a replacement written for KoggerApp.
 * SDL entry point for an app whose Activity, window and input belong to Qt.
 * Only the controller part of SDL is initialized; the audio and activity
 * classes are JNI stubs that exist so libSDL3.so can register its natives.
 */
public class SDL {

    protected static Activity mContext;

    public static void setupJNI() {
        SDLActivity.nativeSetupJNI();
        SDLAudioManager.nativeSetupJNI();
        SDLControllerManager.nativeSetupJNI();
    }

    public static void initialize() {
        setContext(null);
        SDLControllerManager.initialize();
    }

    public static void setContext(Activity context) {
        mContext = context;
    }

    public static Activity getContext() {
        return mContext;
    }
}
