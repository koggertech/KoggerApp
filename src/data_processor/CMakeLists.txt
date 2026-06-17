target_sources(KoggerApp PRIVATE
    bottom_track_processor.h  bottom_track_processor.cpp
    bt_worker.h               bt_worker.cpp
    data_processor.h          data_processor.cpp
    data_processor_defs.h
    isobaths_processor.h      isobaths_processor.cpp
    mosaic_processor.h        mosaic_processor.cpp
    surface_processor.h       surface_processor.cpp
    surface_mesh.h            surface_mesh.cpp
    surface_tile.h            surface_tile.cpp
    compute_worker.h          compute_worker.cpp
    mosaic_db.h               mosaic_db.cpp
    hot_tile_cache.h          hot_tile_cache.cpp)

target_include_directories(KoggerApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
