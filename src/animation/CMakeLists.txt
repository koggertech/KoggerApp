target_sources(KoggerApp PRIVATE
    animation_spec.h
    animation_specs.h
    animator.h  animator.cpp
    smoother.h  smoother.cpp)

target_include_directories(KoggerApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
