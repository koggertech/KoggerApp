# android.pri

android {
    SOURCES += \
        $$PWD/android_init.cpp \
        $$PWD/android_interface.cpp \
        $$PWD/android_serial.cpp \
        $$PWD/kogger_logging_category.cpp \
        $$PWD/qml_object_list_model.cpp

    HEADERS += \
        $$PWD/android_interface.h \
        $$PWD/android_serial.h \
        $$PWD/kogger_logging_category.h \
        $$PWD/qml_object_list_model.h

    INCLUDEPATH += $$PWD

    include($$PWD/qtandroidserialport/qtandroidserialport.pri)
}
