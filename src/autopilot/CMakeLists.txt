target_sources(KoggerApp PRIVATE
    autopilot_messages.h
    frame_codec.h             frame_codec.cpp
    mission_transfer.h        mission_transfer.cpp
    param_transfer.h          param_transfer.cpp
    param_metadata.h          param_metadata.cpp
    autopilot_params.h        autopilot_params.cpp)

target_include_directories(KoggerApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
