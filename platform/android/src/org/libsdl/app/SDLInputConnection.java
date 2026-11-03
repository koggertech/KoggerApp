package org.libsdl.app;

/**
 * Not the original SDL 3.4.2 file: a replacement written for KoggerApp.
 * JNI stub: libSDL3.so registers these natives at load time. Text input
 * belongs to Qt, so nothing calls them.
 */
class SDLInputConnection {

    static native void nativeCommitText(String text, int newCursorPosition);
    static native void nativeGenerateScancodeForUnichar(char c);
}
