# src/scene3d/scene3d.pri

include($$PWD/controllers/controllers.pri)
include($$PWD/core/core.pri)
include($$PWD/domain/domain.pri)
include($$PWD/events/events.pri)
include($$PWD/processors/processors.pri)
include($$PWD/utils/utils.pri)

INCLUDEPATH *= $$PWD
DEPENDPATH *= $$PWD

HEADERS += \
    $$PWD/scene3d_renderer.h \
    $$PWD/scene3d_view.h

SOURCES += \
    $$PWD/scene3d_renderer.cpp \
    $$PWD/scene3d_view.cpp
