package org.libsdl.app;

/**
 * Not the original SDL 3.4.2 file: a replacement written for KoggerApp.
 * JNI stub: libSDL3.so registers these natives and looks up these callbacks
 * at load time. SDL audio is built out, so every callback is a no-op.
 */
class SDLAudioManager {

    static void registerAudioDeviceCallback() {
    }

    static void unregisterAudioDeviceCallback() {
    }

    static void audioSetThreadPriority(boolean recording, int device_id) {
    }

    static native void nativeSetupJNI();
    static native void nativeRemoveAudioDevice(boolean recording, int deviceId);
    static native void nativeAddAudioDevice(boolean recording, String name, int deviceId);
}
