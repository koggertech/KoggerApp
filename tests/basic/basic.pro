CONFIG += testcase
QT += testlib core gui

TARGET = tst_basic

HEADERS += \
    tst_basic.h

SOURCES += \
    tst_basic.cpp

RESOURCES += \
    tst_basic.qrc


include($$TOP_PWD/KoggerApp/core/core.pri)
include($$TOP_PWD/KoggerApp/processors/processors.pri)
include($$TOP_PWD/KoggerApp/domain/domain.pri)
