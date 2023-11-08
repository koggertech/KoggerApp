QT += quick
QT += widgets
QT += network
QT += qml

windows {
    QT += serialport
}

android {
    QT += androidextras
    QT += core-private
    CONFIG += mobility
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
        IDBinnary.cpp \
    Link.cpp \
        ProtoBinnary.cpp \
        StreamListModel.cpp \
    bottomtrackprovider.cpp \
        connection.cpp \
        console.cpp \
        consolelistmodel.cpp \
        core.cpp \
#        coreFlash.cpp \
        filelist.cpp \
    graphicsscene3drenderer.cpp \
    graphicsscene3dview.cpp \
    interpolatorbase.cpp \
        logger.cpp \
        main.cpp \
        flasher.cpp \
    maxpointsfilter.cpp \
    nearestpointfilter.cpp \
        plotcash.cpp \
    raycaster.cpp \
        streamlist.cpp \
        waterfall.cpp \


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
QML_IMPORT_PATH = $$PWD\QML

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH = $$PWD\QML

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    3Plot.h \
    DevDriver.h \
    DevHub.h \
    DevQProperty.h \
    IDBinnary.h \
    Link.h \
    MAVLinkConf.h \
    ProtoBinnary.h \
    StreamListModel.h \
    Themes.h \
    abstractentitydatafilter.h \
    bottomtrackprovider.h \
    connection.h \
    console.h \
    consolelistmodel.h \
    filelist.h \
    flasher.h \
    core.h \
    graphicsscene3drenderer.h \
    graphicsscene3dview.h \
    interpolatorbase.h \
    logger.h \
    maxpointsfilter.h \
    nearestpointfilter.h \
    plotcash.h \
    raycaster.h \
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
    QML/Common/MenuBlockEx.qml \
    QML/Scene3DToolbar.qml \
    QML/SceneObjectsControlBar/ActiveObjectParams.qml \
    QML/SceneObjectsControlBar/BottomTrackParams.qml \
    QML/SceneObjectsControlBar/SceneObjectsControlBar.qml \
    QML/SceneObjectsControlBar/SceneObjectsList.qml \
    QML/SceneObjectsControlBar/SceneObjectsListDelegate.qml \
    QML/SceneObjectsList.qml \
    QML/SceneObjectsListDelegate.qml \
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
    base.vsh \
    heightcolor.frag \
    staticcolor.fsh \
    tools/models.pri \
    tools/tools.pri


android {
DISTFILES += \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat \
    android/res/values/libs.xml \
    qtandroidserialport/src/qtandroidserialport.pri
}

android {
    ANDROID_ABIS = armeabi-v7a
}

LIBS += -lopengl32

include ($$PWD/core/core.pri)
include ($$PWD/processors/processors.pri)
include ($$PWD/domain/domain.pri)
include ($$PWD/controllers/controllers.pri)

#ANDROID_ABIS = x86

#ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

ANDROID_ABIS = armeabi-v7a

