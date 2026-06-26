target_sources(KoggerApp PRIVATE
    qml_object_names.h
    scene_object.h      scene_object.cpp)

target_include_directories(KoggerApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
