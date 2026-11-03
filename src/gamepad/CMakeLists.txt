target_sources(KoggerApp PRIVATE
    gamepad_monitor.h         gamepad_monitor.cpp
    gamepad_worker.h          gamepad_worker.cpp)

target_include_directories(KoggerApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
