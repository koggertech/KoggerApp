cmake_minimum_required(VERSION 3.22)

project(KoggerApp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

list(APPEND QT_SKIP_AUTO_PLUGIN_INCLUSION
    QMYSQLDriverPlugin
    QMimerSQLDriverPlugin
    QODBCDriverPlugin
    QPSQLDriverPlugin)

find_package(Qt6 REQUIRED COMPONENTS
    Core Gui Qml Quick QuickControls2 Network Sql Concurrent Positioning Svg)

qt_standard_project_setup(REQUIRES 6.8)

set(CMAKE_AUTOUIC OFF)

option(KOGGER_FLASHER          "Build device flasher"                OFF)
option(KOGGER_SEPARATE_READING "Data reception in a separate thread" OFF)
option(KOGGER_SCENE_TESTING    "3D scene testing"                    OFF)
option(KOGGER_MANUAL_TESTING   "Manual testing hooks"                OFF)

qt_add_executable(KoggerApp
    src/main.cpp

    src/black_stripes_processor.h  src/black_stripes_processor.cpp
    src/console.h                  src/console.cpp
    src/console_list_model.h       src/console_list_model.cpp
    src/converter_xtf.h
    src/core.h                     src/core.cpp
    src/internet_manager.h         src/internet_manager.cpp
    src/dataset.h                  src/dataset.cpp
    src/dataset_defs.h
    src/data_interpolator.h        src/data_interpolator.cpp
    src/data_horizon.h             src/data_horizon.cpp
    src/dsp_defs.h
    src/epoch.h                    src/epoch.cpp
    src/hotkeys_manager.h          src/hotkeys_manager.cpp
    src/hotkeys_controller.h       src/hotkeys_controller.cpp
    src/id_binnary.h               src/id_binnary.cpp
    src/delaunay.h
    src/delaunay_defs.h
    src/logger.h                   src/logger.cpp
    src/map_defs.h                 src/map_defs.cpp
    src/isobaths_defs.h
    src/math_defs.h
    src/mav_link_conf.h
    src/proto_binnary.h
    src/stream_list.h              src/stream_list.cpp
    src/stream_list_model.h        src/stream_list_model.cpp
    src/themes.h
    src/language_controller.h
    src/ui_state_serializer.h      src/ui_state_serializer.cpp
    src/echogram_state_serializer.h src/echogram_state_serializer.cpp
    src/notifications.h            src/notifications.cpp
    src/input_device_tracker.h     src/input_device_tracker.cpp
    src/xtf_conf.h
    src/location_reader.h          src/location_reader.cpp
    src/mosaic_index_provider.h    src/mosaic_index_provider.cpp
    src/app_utils.h
    src/ui_keepalive.h             src/ui_keepalive.cpp
)

target_compile_features(KoggerApp PRIVATE cxx_std_23)

target_include_directories(KoggerApp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/freetype/include)

# C++ modules (each appends target_sources + include dirs to KoggerApp)
add_subdirectory(src/data_processor)
add_subdirectory(src/scene2d)
add_subdirectory(src/scene3d)
add_subdirectory(src/device)
add_subdirectory(src/link)
add_subdirectory(src/tile_engine)

# Optional build flags (off by default)
if(KOGGER_FLASHER)
    target_compile_definitions(KoggerApp PRIVATE FLASHER)
    target_sources(KoggerApp PRIVATE
        src/flasher/flasher.h        src/flasher/flasher.cpp
        src/flasher/deviceflasher.h  src/flasher/deviceflasher.cpp)
endif()
if(KOGGER_SEPARATE_READING)
    target_compile_definitions(KoggerApp PRIVATE SEPARATE_READING)
endif()
if(KOGGER_SCENE_TESTING)
    target_compile_definitions(KoggerApp PRIVATE SCENE_TESTING)
endif()
if(KOGGER_MANUAL_TESTING)
    target_compile_definitions(KoggerApp PRIVATE MANUAL_TESTING)
endif()

target_link_libraries(KoggerApp PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2
    Qt6::Network Qt6::Sql Qt6::Concurrent Qt6::Positioning Qt6::Svg)

target_sources(KoggerApp PRIVATE
    qml.qrc
    resources/icons.qrc
    resources/resources.qrc)

# ── Platform branches ────────────────────────────────────────────────────
if(ANDROID)
    add_subdirectory(platform/android/src)
elseif(WIN32)
    set_target_properties(KoggerApp PROPERTIES WIN32_EXECUTABLE ON)
    find_package(Qt6 REQUIRED COMPONENTS Widgets SerialPort)
    target_link_libraries(KoggerApp PRIVATE Qt6::Widgets Qt6::SerialPort opengl32
        "${CMAKE_SOURCE_DIR}/third_party/freetype/lib/llvm-mingw-x64/libfreetype.a")
    target_sources(KoggerApp PRIVATE resources/shaders.qrc resources/file.rc)
elseif(UNIX)
    find_package(Qt6 REQUIRED COMPONENTS Widgets SerialPort)
    target_link_libraries(KoggerApp PRIVATE Qt6::Widgets Qt6::SerialPort)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm")
        target_compile_definitions(KoggerApp PRIVATE LINUX_ES)
        target_link_libraries(KoggerApp PRIVATE GLESv2 png brotlidec
            "${CMAKE_SOURCE_DIR}/third_party/freetype/lib/aarch64/libfreetype.a")
        target_sources(KoggerApp PRIVATE platform/android/shaders.qrc)
    else()
        target_compile_definitions(KoggerApp PRIVATE LINUX_DESKTOP)
        target_link_libraries(KoggerApp PRIVATE GL
            "${CMAKE_SOURCE_DIR}/third_party/freetype/lib/gcc/libfreetype.a")
        target_sources(KoggerApp PRIVATE resources/shaders.qrc)
    endif()
endif()
