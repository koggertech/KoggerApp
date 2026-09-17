target_sources(KoggerApp PRIVATE
    control_defs.h
    control_listener.h     control_listener.cpp
    command_dispatcher.h   command_dispatcher.cpp
    control_server.h       control_server.cpp)

target_include_directories(KoggerApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
