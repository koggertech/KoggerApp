target_sources(KoggerApp PRIVATE
    autopilot_messages.h
    frame_codec.h             frame_codec.cpp
    mission_transfer.h        mission_transfer.cpp)

target_include_directories(KoggerApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
