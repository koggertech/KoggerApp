target_sources(KoggerApp PRIVATE
    tile_db.h               tile_db.cpp
    tile_downloader.h       tile_downloader.cpp
    tile_google_provider.h  tile_google_provider.cpp
    tile_osm_provider.h     tile_osm_provider.cpp
    tile_baidu_provider.h   tile_baidu_provider.cpp
    tile_manager.h          tile_manager.cpp
    tile_provider.h         tile_provider.cpp
    tile_provider_ids.h
    tile_set.h              tile_set.cpp)

target_include_directories(KoggerApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
