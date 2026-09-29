target_sources(KoggerApp PRIVATE
    mission_defs.h
    mission_model.h
    mission_geometry.h        mission_geometry.cpp
    mission_generators.h      mission_generators.cpp
    mission_expander.h        mission_expander.cpp
    mission_file_io.h         mission_file_io.cpp
    mission_export.h          mission_export.cpp
    mission_plan_controller.h mission_plan_controller.cpp)

target_include_directories(KoggerApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
