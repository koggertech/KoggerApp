INCLUDEPATH *= $$PWD
DEPENDPATH *= $$PWD

HEADERS *= \
	$$PWD/dev_driver.h\
    $$PWD/dev_q_property.h \
	$$PWD/device_defs.h \
	$$PWD/device_manager.h \
	$$PWD/device_manager_wrapper.h \

SOURCES *= \
    $$PWD/dev_driver.cpp \
	$$PWD/device_manager.cpp \
	$$PWD/device_manager_wrapper.cpp