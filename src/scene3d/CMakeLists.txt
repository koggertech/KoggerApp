add_subdirectory(controllers)
add_subdirectory(core)
add_subdirectory(domain)
add_subdirectory(events)
add_subdirectory(utils)

target_sources(KoggerApp PRIVATE
    scene3d_renderer.h  scene3d_renderer.cpp
    scene3d_view.h      scene3d_view.cpp)

target_include_directories(KoggerApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
