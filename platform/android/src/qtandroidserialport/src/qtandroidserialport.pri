android {
    INCLUDEPATH *= $$PWD

    PUBLIC_HEADERS += \
        $$PWD/qserialport.h \
        $$PWD/qserialportinfo.h \
        $$PWD/qringbuffer_p.h \
        $$PWD/qglobal_p.h \
        $$PWD/qconfig_p.h \
        $$PWD/qtcore-config_p.h

    PRIVATE_HEADERS += \
        $$PWD/qserialport_p.h \
        $$PWD/qserialportinfo_p.h \
        $$PWD/qserialport_android_p.h

    SOURCES += \
        $$PWD/qserialport.cpp \
        $$PWD/qserialportinfo.cpp \
        $$PWD/qserialport_android.cpp \
        $$PWD/qserialportinfo_android.cpp

    HEADERS += $$PUBLIC_HEADERS $$PRIVATE_HEADERS
}
