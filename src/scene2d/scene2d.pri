# src/scene3d/scene2d.pri

INCLUDEPATH *= $$PWD
DEPENDPATH *= $$PWD

HEADERS += \
    $$PWD/plot2D.h \
    $$PWD/plot2D_aim.h \
    $$PWD/plot2D_attitude.h \
    $$PWD/plot2D_bottom_processing.h \
    $$PWD/plot2D_contact.h \
    $$PWD/plot2D_defs.h \
    $$PWD/plot2D_dvl_beam_velocity.h \
    $$PWD/plot2D_dvl_solution.h \
    $$PWD/plot2D_echogram.h \
    $$PWD/plot2D_encoder.h \
    $$PWD/plot2D_gnss.h \
    $$PWD/plot2D_grid.h \
    $$PWD/plot2D_line.h \
    $$PWD/plot2D_plot_layer.h \
    $$PWD/plot2D_quadrature.h \
    $$PWD/plot2D_rangefinder.h \
    $$PWD/plot2D_usbl_solution.h \
    $$PWD/qPlot2D.h

SOURCES += \
    $$PWD/plot2D.cpp \
    $$PWD/plot2D_aim.cpp \
    $$PWD/plot2D_attitude.cpp \
    $$PWD/plot2D_bottom_processing.cpp \
    $$PWD/plot2D_contact.cpp \
    $$PWD/plot2D_dvl_beam_velocity.cpp \
    $$PWD/plot2D_dvl_solution.cpp \
    $$PWD/plot2D_echogram.cpp \
    $$PWD/plot2D_encoder.cpp \
    $$PWD/plot2D_gnss.cpp \
    $$PWD/plot2D_grid.cpp \
    $$PWD/plot2D_line.cpp \
    $$PWD/plot2D_quadrature.cpp \
    $$PWD/plot2D_rangefinder.cpp \
    $$PWD/plot2D_usbl_solution.cpp \
    $$PWD/qPlot2D.cpp
