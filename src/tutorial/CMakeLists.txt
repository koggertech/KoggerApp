target_sources(KoggerApp PRIVATE
    tutorial.h                tutorial.cpp)

qt_add_resources(KoggerApp "kogger_tutorial_scenarios"
    PREFIX "/tutorial"
    BASE "${CMAKE_SOURCE_DIR}/resources/tutorial"
    FILES
        "${CMAKE_SOURCE_DIR}/resources/tutorial/demo.json"
        "${CMAKE_SOURCE_DIR}/resources/tutorial/missions.json")

target_include_directories(KoggerApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
