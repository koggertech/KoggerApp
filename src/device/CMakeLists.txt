target_sources(KoggerApp PRIVATE
    dev_driver.h              dev_driver.cpp
    dev_q_property.h
    device_defs.h
    device_manager.h          device_manager.cpp
    device_manager_wrapper.h  device_manager_wrapper.cpp)

target_include_directories(KoggerApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
