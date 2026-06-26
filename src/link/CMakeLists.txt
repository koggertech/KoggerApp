target_sources(KoggerApp PRIVATE
    link.h                    link.cpp
    link_defs.h
    link_list_model.h         link_list_model.cpp
    link_manager.h            link_manager.cpp
    link_manager_wrapper.h    link_manager_wrapper.cpp)

target_include_directories(KoggerApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
