INCLUDEPATH *= $$PWD
DEPENDPATH *= $$PWD

HEADERS *= \
    $$PWD/tile_db.h \
	$$PWD/tile_downloader.h \
	$$PWD/tile_google_provider.h \
	$$PWD/tile_manager.h \
	$$PWD/tile_provider.h \
	$$PWD/tile_set.h

SOURCES *= \
    $$PWD/tile_db.cpp \
	$$PWD/tile_downloader.cpp \
	$$PWD/tile_google_provider.cpp \
	$$PWD/tile_manager.cpp \
	$$PWD/tile_provider.cpp \
	$$PWD/tile_set.cpp
