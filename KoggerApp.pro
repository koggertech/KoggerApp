QT += quick
QT += widgets
QT += network
QT += qml

#CONFIG += FLASHER

windows {
    QT += serialport
}

android {
    QT += androidextras
    QT += core-private
    CONFIG += mobility

    QMAKE_CXXFLAGS_DEBUG -= -O2
    QMAKE_CXXFLAGS_DEBUG -= -O3
    QMAKE_CXXFLAGS_DEBUG += -O0
}

CONFIG += c++11


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

SOURCES += \
        3Plot.cpp \
        DevDriver.cpp \
        DevHub.cpp \
    EchogramProcessing.cpp \
        IDBinnary.cpp \
    Link.cpp \
    Plot2D.cpp \
    Plot2DEchogram.cpp \
    Plot2DGrid.cpp \
        ProtoBinnary.cpp \
        StreamListModel.cpp \
        connection.cpp \
        console.cpp \
        consolelistmodel.cpp \
        core.cpp \
        filelist.cpp \
    interpolatorbase.cpp \
        logger.cpp \
        main.cpp \
        flasher.cpp \
        plotcash.cpp \
        streamlist.cpp \
        waterfall.cpp \

FLASHER {
DEFINES += FLASHER
SOURCES += coreFlash.cpp
}

android {
SOURCES += \
    android.cpp \
    qtandroidserialport/src/qserialport.cpp \
    qtandroidserialport/src/qserialport_android.cpp \
    qtandroidserialport/src/qserialportinfo.cpp \
    qtandroidserialport/src/qserialportinfo_android.cpp \
}

RESOURCES += QML/qml.qrc \
    shaders.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    3Plot.h \
    ConverterXTF.h \
    DevDriver.h \
    DevHub.h \
    DevQProperty.h \
    EchogramProcessing.h \
    IDBinnary.h \
    Link.h \
    MAVLinkConf.h \
    Plot2D.h \
    ProtoBinnary.h \
    StreamListModel.h \
    Themes.h \
    XTFConf.h \
    connection.h \
    console.h \
    consolelistmodel.h \
    filelist.h \
    flasher.h \
    core.h \
    interpolatorbase.h \
    logger.h \
    plotcash.h \
    streamlist.h \
    waterfall.h \
    waterfallproxy.h \

android {
HEADERS += \
    android.h \
    qtandroidserialport/src/qserialport_android_p.h \
    qtandroidserialport/src/qserialport_p.h \
    qtandroidserialport/src/qserialport.h \
    qtandroidserialport/src/qserialportinfo.h \
    qtandroidserialport/src/qserialportinfo_p.h
}


DISTFILES += \
    QML/Settings3DView.qml \
    QML/AdjBox.qml \
    QML/AdjBoxBack.qml \
    QML/BackStyle.qml \
    QML/ButtonBackStyle.qml \
    QML/CButton.qml \
    QML/CCombo.qml \
    QML/CComboBox.qml \
    QML/CSlider.qml \
    QML/ComboBackStyle.qml \
    QML/ConnectionViewer.qml \
    QML/Console.qml \
    QML/CustomGroupBox.qml \
    QML/DeviceSettingsViewer.qml \
    QML/MenuBar.qml \
    QML/MenuButton.qml \
    QML/MenuViewer.qml \
    QML/TabBackStyle.qml \
    QML/UpgradeBox.qml \
    QML/FlashBox.qml \
    QML/main.qml \
    a.fsh \
    android_build/AndroidManifest.xml \
    android_build/build.gradle \
    android_build/gradle.properties \
    android_build/gradle/wrapper/gradle-wrapper.jar \
    android_build/gradle/wrapper/gradle-wrapper.properties \
    android_build/gradlew \
    android_build/gradlew.bat \
    android_build/res/values/libs.xml \
    base.vsh \
    heightcolor.frag \
    staticcolor.fsh


android {
DISTFILES += \
    android_build/AndroidManifest.xml \
    android_build/build.gradle \
    android_build/gradle/wrapper/gradle-wrapper.jar \
    android_build/gradle/wrapper/gradle-wrapper.properties \
    android_build/gradlew \
    android_build/gradlew.bat \
    android_build/res/values/libs.xml \
    qtandroidserialport/src/qtandroidserialport.pri
}



windows {
    LIBS += -lopengl32
}

include ($$PWD/core/core.pri)
include ($$PWD/factories/factories.pri)
include ($$PWD/processors/processors.pri)
include ($$PWD/domain/domain.pri)
include ($$PWD/models/models.pri)
include ($$PWD/controllers/controllers.pri)


android {
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android_build
#    ANDROID_ABIS = arm64-v8a
##    ANDROID_ABIS = x86
}

ANDROID_ABIS = armeabi-v7a


android: include(C:/Users/aproo/AppData/Local/Android/Sdk/android_openssl/openssl.pri)
