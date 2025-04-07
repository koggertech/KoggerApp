QT += quick
QT += widgets
QT += network
QT += qml
QT += sql

#CONFIG += FLASHER
#CONFIG += MOTOR # motor_control definition
#CONFIG += SEPARATE_READING # data reception in a separate thread
#CONFIG += FAKE_COORDS

!android {
    QT += serialport
}

android {
    ANDROID_TARGET_SDK_VERSION = 34

    QT += androidextras
    QT += core-private
    QT += gui-private
    QT += svg

    CONFIG += mobility

    QMAKE_CXXFLAGS_DEBUG -= -O2
    QMAKE_CXXFLAGS_DEBUG -= -O3
    QMAKE_CXXFLAGS_DEBUG += -O0
}

CONFIG += c++17


CONFIG += qmltypes
QML_IMPORT_NAME = SceneGraphRendering
QML_IMPORT_MAJOR_VERSION = 1

#QMAKE_CXXFLAGS_RELEASE += -02

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Refer to the documentation for the
# deprecated API to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS


# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


### SOURCES
SOURCES += \
    src/black_stripes_processor.cpp \
    src/console.cpp \
    src/console_list_model.cpp \
    src/core.cpp \
    src/flasher.cpp \
    src/hotkeys_manager.cpp \
    src/id_binnary.cpp \
    src/logger.cpp \
    src/main.cpp \
    src/map_defs.cpp \
    src/plot2D.cpp \
    src/plot2D_echogram.cpp \
    src/plot2D_grid.cpp \
    src/plotcash.cpp \
    src/proto_binnary.cpp \
    src/stream_list.cpp \
    src/stream_list_model.cpp \
    src/waterfall.cpp


FLASHER {
DEFINES += FLASHER
SOURCES += src/coreFlash.cpp
}

SEPARATE_READING {
DEFINES += SEPARATE_READING
}
FAKE_COORDS {
DEFINES += FAKE_COORDS
}

android {
SOURCES += \
    platform/android/src/android.cpp \
    platform/android/src/qtandroidserialport/src/qserialport.cpp \
    platform/android/src/qtandroidserialport/src/qserialport_android.cpp \
    platform/android/src/qtandroidserialport/src/qserialportinfo.cpp \
    platform/android/src/qtandroidserialport/src/qserialportinfo_android.cpp \
}

TRANSLATIONS += translations/translation_en.ts \
                translations/translation_ru.ts \
                translations/translation_pl.ts

RESOURCES += qml/qml.qrc \
    resources/icons.qrc \
    resources/resources.qrc

windows {
    message("Building for Windows with full OpenGL")
    LIBS += -lopengl32
    RESOURCES += resources/shaders.qrc
}
linux:!android {
    PLATFORM_ARCH = $$system(uname -m)
    equals(PLATFORM_ARCH, aarch64) {
        message("Building for Raspberry Pi (ARM) with OpenGL ES")
        #DEFINES += USE_OPENGLES
        DEFINES += LINUX_ES
        LIBS += -lGLESv2
        RESOURCES += platform/android/shaders.qrc
    } else {
        message("Building for Ubuntu Desktop with full OpenGL")
        DEFINES += LINUX_DESKTOP
        LIBS += -lGL
        RESOURCES += resources/shaders.qrc
    }
}

android {
    message("Building for Android (ARM) with OpenGL ES")
    RESOURCES += platform/android/shaders.qrc
}

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH = $$PWD\qml

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH = $$PWD\qml

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


### HEADERS
HEADERS += \
    src/black_stripes_processor.h \
    src/console.h \
    src/console_list_model.h \
    src/converter_xtf.h \
    src/core.h \
    src/dsp_defs.h \
    src/flasher.h \
    src/hotkeys_manager.h \
    src/id_binnary.h \
    src/logger.h \
    src/map_defs.h \
    src/mav_link_conf.h \
    src/plot2D.h \
    src/plotcash.h \
    src/proto_binnary.h \
    src/stream_list.h \
    src/stream_list_model.h \
    src/themes.h \
    src/waterfall.h \
    src/xtf_conf.h

android {
HEADERS += \
    platform/android/src/android.h \
    platform/android/src/qtandroidserialport/src/qserialport_android_p.h \
    platform/android/src/qtandroidserialport/src/qserialport_p.h \
    platform/android/src/qtandroidserialport/src/qserialport.h \
    platform/android/src/qtandroidserialport/src/qserialportinfo.h \
    platform/android/src/qtandroidserialport/src/qserialportinfo_p.h
}


### DISTFILES
DISTFILES += \
    qml/Common/MenuBlockEx.qml \
    qml/Scene3DToolbar.qml \
    qml/SceneObjectsControlBar/ActiveObjectParams.qml \
    qml/SceneObjectsControlBar/BottomTrackParams.qml \
    qml/SceneObjectsControlBar/SceneObjectsControlBar.qml \
    qml/SceneObjectsControlBar/SceneObjectsList.qml \
    qml/SceneObjectsControlBar/SceneObjectsListDelegate.qml \
    qml/SceneObjectsList.qml \
    qml/SceneObjectsListDelegate.qml \
    qml/AdjBox.qml \
    qml/AdjBoxBack.qml \
    qml/BackStyle.qml \
    qml/ButtonBackStyle.qml \
    qml/CButton.qml \
    qml/CCombo.qml \
    qml/CComboBox.qml \
    qml/CSlider.qml \
    qml/ComboBackStyle.qml \
    qml/ConnectionViewer.qml \
    qml/Console.qml \
    qml/CustomGroupBox.qml \
    qml/DeviceSettingsViewer.qml \
    qml/MenuBar.qml \
    qml/MenuFrame.qml \
    qml/MenuButton.qml \
    qml/MenuViewer.qml \
    qml/TabBackStyle.qml \
    qml/UpgradeBox.qml \
    qml/FlashBox.qml \
    qml/main.qml \
    platform/android/AndroidManifest.xml \
    platform/android/build.gradle \
    platform/android/gradle.properties \
    platform/android/gradle/wrapper/gradle-wrapper.jar \
    platform/android/gradle/wrapper/gradle-wrapper.properties \
    platform/android/gradlew \
    platform/android/gradlew.bat \
    platform/android/res/values/libs.xml


android {
DISTFILES += \
    platform/android/AndroidManifest.xml \
    platform/android/build.gradle \
    platform/android/gradle/wrapper/gradle-wrapper.jar \
    platform/android/gradle/wrapper/gradle-wrapper.properties \
    platform/android/gradlew \
    platform/android/gradlew.bat \
    platform/android/res/values/libs.xml \
    platform/android/src/qtandroidserialport/src/qtandroidserialport.pri
}

win32:RC_FILE = resources/file.rc

android {
    equals(ANDROID_TARGET_ARCH, arm64-v8a) {
        message("Adding FreeType Lib for arm64-v8a arch")
        LIBS += -L$$PWD/third_party/freetype/lib/arm64-v8a -lfreetype
    } else:equals(ANDROID_TARGET_ARCH, armeabi-v7a) {
        message("Adding FreeType Lib for armeabi-v7a arch")
        LIBS += -L$$PWD/third_party/freetype/lib/armeabi-v7a -lfreetype
    }
}

linux:!android {
    contains(QMAKE_HOST.arch, arm) {
        message("Using freetype for Raspberry Pi 4 (aarch64)")
        LIBS += -L$$PWD/third_party/freetype/lib/aarch64 -lfreetype
        LIBS += -lpng -lbrotlidec
    }
    else {
        LIBS += -L$$PWD/third_party/freetype/lib/gcc/ -lfreetype
    }
}

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/third_party/freetype/lib/mingw-x64/ -lfreetype
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/third_party/freetype/lib/mingw-x64/ -lfreetype
#else:unix:!macx: LIBS += -L$$PWD/third_party/freetype/lib/gcc/ -lfreetype
#else:unix:!android: LIBS += -L$$PWD/third_party/freetype/lib/gcc/ -lfreetype

INCLUDEPATH += $$PWD/third_party/freetype/include
INCLUDEPATH += $$PWD/src
DEPENDPATH += $$PWD/third_party/freetype/include


include($$PWD/src/scene3d/scene3d.pri)
include($$PWD/src/device/device_manager.pri)
include($$PWD/src/link/link.pri)
include($$PWD/src/tile_engine/tile_engine.pri)


android {
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/platform/android
#    ANDROID_ABIS = arm64-v8a
##    ANDROID_ABIS = x86
}

ANDROID_ABIS = armeabi-v7a arm64-v8a

android {
    OPENSSL_PATH = $$ANDROID_SDK_ROOT/android_openssl/openssl.pri
    include($$OPENSSL_PATH)
}

MOTOR {
DEFINES += MOTOR
HEADERS += src/motor_control.h
SOURCES += src/motor_control.cpp
DISTFILES += qml/MotorViewer.qml
}
