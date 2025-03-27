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
    src/3Plot.cpp \
    src/DevDriver.cpp \
    src/DeviceManager.cpp \
    src/DeviceManagerWrapper.cpp \
    src/EchogramProcessing.cpp \
    src/IDBinnary.cpp \
    src/Link.cpp \
    src/LinkManager.cpp \
    src/LinkManagerWrapper.cpp \
    src/Plot2D.cpp \
    src/Plot2DEchogram.cpp \
    src/Plot2DGrid.cpp \
    src/ProtoBinnary.cpp \
    src/LinkListModel.cpp \
    src/StreamListModel.cpp \
    src/console.cpp \
    src/consolelistmodel.cpp \
    src/core.cpp \
    src/filelist.cpp \
    src/geometryengine.cpp \
    src/graphicsscene3drenderer.cpp \
    src/graphicsscene3dview.cpp \
    src/logger.cpp \
    src/main.cpp \
    src/flasher.cpp \
    src/maxpointsfilter.cpp \
    src/nearestpointfilter.cpp \
    src/plotcash.cpp \
    src/ray.cpp \
    src/raycaster.cpp \
    src/streamlist.cpp \
    src/textrenderer.cpp \
    src/waterfall.cpp \
    src/tile_manager.cpp \
    src/tile_set.cpp \
    src/tile_provider.cpp \
    src/tile_google_provider.cpp \
    src/tile_downloader.cpp \
    src/tile_db.cpp \
    src/map_defs.cpp \
    src/hotkeys_manager.cpp \
    src/connection.cpp

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
    src/3Plot.h \
    src/ConverterXTF.h \
    src/DSP.h \
    src/DevDriver.h \
    src/DeviceManager.h \
    src/DeviceManagerWrapper.h \
    src/DevQProperty.h \
    src/EchogramProcessing.h \
    src/IDBinnary.h \
    src/Link.h \
    src/LinkManager.h \
    src/LinkManagerWrapper.h \
    src/MAVLinkConf.h \
    src/Plot2D.h \
    src/ProtoBinnary.h \
    src/LinkListModel.h \
    src/StreamListModel.h \
    src/Themes.h \
    src/abstractentitydatafilter.h \
    src/XTFConf.h \
    src/console.h \
    src/consolelistmodel.h \
    src/filelist.h \
    src/flasher.h \
    src/core.h \
    src/geometryengine.h \
    src/graphicsscene3drenderer.h \
    src/graphicsscene3dview.h \
    src/logger.h \
    src/maxpointsfilter.h \
    src/nearestpointfilter.h \
    src/plotcash.h \
    src/ray.h \
    src/raycaster.h \
    src/streamlist.h \
    src/textrenderer.h \
    src/waterfall.h \
    src/waterfallproxy.h \
    src/tile_manager.h \
    src/tile_set.h \
    src/tile_provider.h \
    src/tile_google_provider.h \
    src/tile_downloader.h \
    src/tile_db.h \
    src/map_defs.h \
    src/hotkeys_manager.h \
    src/connection.h

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

include ($$PWD/src/core/core.pri)
include ($$PWD/src/processors/processors.pri)
include ($$PWD/src/domain/domain.pri)
include ($$PWD/src/controllers/controllers.pri)
include ($$PWD/src/events/events.pri)


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
