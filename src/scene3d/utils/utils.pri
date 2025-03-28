INCLUDEPATH *= $$PWD
DEPENDPATH *= $$PWD

HEADERS += \
    $$PWD/abstract_entity_data_filter.h \
    $$PWD/max_points_filter.h \
    $$PWD/nearest_point_filter.h \
    $$PWD/ray.h \
    $$PWD/ray_caster.h \
    $$PWD/text_renderer.h

SOURCES += \
    $$PWD/max_points_filter.cpp \
    $$PWD/nearest_point_filter.cpp \
    $$PWD/ray.cpp \
    $$PWD/ray_caster.cpp \
    $$PWD/text_renderer.cpp
