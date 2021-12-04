QT += quick
QT += serialport
QT += widgets
QT += network

CONFIG += c++11

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
        DevDriver.cpp \
        DevHub.cpp \
        IDBinnary.cpp \
        ProtoBinnary.cpp \
        StreamListModel.cpp \
        connection.cpp \
        console.cpp \
        consolelistmodel.cpp \
        core.cpp \
        filelist.cpp \
        logger.cpp \
        main.cpp \
        coreFlash.cpp \
        flasher.cpp \
        plotcash.cpp \
        streamlist.cpp \
        waterfall.cpp

RESOURCES += QML/qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    DevDriver.h \
    DevHub.h \
    DevQProperty.h \
    IDBinnary.h \
    ProtoBinnary.h \
    StreamListModel.h \
    Themes.h \
    connection.h \
    console.h \
    consolelistmodel.h \
    filelist.h \
    flasher.h \
    core.h \
    logger.h \
    plotcash.h \
    streamlist.h \
    waterfall.h \
    waterfallproxy.h

DISTFILES += \
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
    QML/main.qml

ANDROID_ABIS = armeabi-v7a

#ANDROID_ABIS = x86
