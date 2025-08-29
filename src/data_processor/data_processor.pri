INCLUDEPATH *= $$PWD
DEPENDPATH *= $$PWD

HEADERS *= \
    $$PWD/bottom_track_processor.h \
    $$PWD/data_processor.h \
    $$PWD/data_processor_defs.h \
    $$PWD/isobaths_processor.h \
    $$PWD/mosaic_processor.h \
    $$PWD/surface_processor.h \
    $$PWD/surface_mesh.h \
    $$PWD/surface_tile.h \
    $$PWD/compute_worker.h

SOURCES *= \
    $$PWD/bottom_track_processor.cpp \
    $$PWD/data_processor.cpp \
    $$PWD/isobaths_processor.cpp \
    $$PWD/mosaic_processor.cpp \
    $$PWD/surface_processor.cpp \
    $$PWD/surface_mesh.cpp \
    $$PWD/surface_tile.cpp \
    $$PWD/compute_worker.cpp
