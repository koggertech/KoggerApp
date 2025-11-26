# qtandroidserialport/qtandroidserialport.pri

android {
    QT += core-private

    SOURCES += \
        $$PWD/qserialport.cpp \
        $$PWD/qserialport_android.cpp \
        $$PWD/qserialportinfo.cpp \
        $$PWD/qserialportinfo_android.cpp

    HEADERS += \
        $$PWD/qserialport.h \
        $$PWD/qserialport_p.h \
        $$PWD/qserialportglobal.h \
        $$PWD/qserialportinfo.h \
        $$PWD/qserialportinfo_p.h \
        $$PWD/qtserialportexports.h \
        $$PWD/qtserialportversion.h

    INCLUDEPATH += $$PWD
}
