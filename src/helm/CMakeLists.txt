target_sources(KoggerApp PRIVATE
    helm_shaping.h
    helm_bindings.h
    helm_controller.h         helm_controller.cpp)

target_include_directories(KoggerApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
