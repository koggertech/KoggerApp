cmake_minimum_required(VERSION 3.22)

set(KOGGER_VERSION "0.0.0")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/version.txt")
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/resources/version.txt" _kver LIMIT_COUNT 1)
    string(STRIP "${_kver}" _kver)
    if(_kver MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
        set(KOGGER_VERSION "${_kver}")
    endif()
endif()
string(REPLACE "." ";" _kvp "${KOGGER_VERSION}")
list(GET _kvp 0 _kvmaj)
list(GET _kvp 1 _kvmin)
list(GET _kvp 2 _kvpat)
math(EXPR KOGGER_VERSION_CODE "${_kvmaj} * 10000 + ${_kvmin} * 100 + ${_kvpat}")

project(KoggerApp VERSION ${KOGGER_VERSION} LANGUAGES CXX)

if(ANDROID)
    set(QT_ANDROID_ABIS "armeabi-v7a;arm64-v8a" CACHE STRING "Android target ABIs" FORCE)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    file(GLOB _gcc_dirs LIST_DIRECTORIES true /usr/lib/gcc/x86_64-linux-gnu/*)
    list(SORT _gcc_dirs)
    list(REVERSE _gcc_dirs)
    if(_gcc_dirs)
        list(GET _gcc_dirs 0 _gcc_dir)
        if(IS_DIRECTORY "${_gcc_dir}")
            add_compile_options(--gcc-install-dir=${_gcc_dir})
        endif()
    endif()
endif()

list(APPEND QT_SKIP_AUTO_PLUGIN_INCLUSION
    QMYSQLDriverPlugin
    QMimerSQLDriverPlugin
    QODBCDriverPlugin
    QPSQLDriverPlugin)

find_package(Qt6 REQUIRED COMPONENTS
    Core Gui Qml Quick QuickControls2 Network Sql Concurrent Positioning Svg LinguistTools)

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
add_subdirectory(src/animation)
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
    resources/icons.qrc
    resources/resources.qrc)

qt_add_resources(KoggerApp "kogger_main_qml"
    PREFIX "/"
    FILES qml/main.qml)

# ── QML modules (one CMakeLists per dir, STATIC; RESOURCE_PREFIX /qml → :/qml/<uri>/) ──
add_subdirectory(qml/kqml_types)
add_subdirectory(qml/controls)
add_subdirectory(qml/menus)
add_subdirectory(qml/settings)
add_subdirectory(qml/devices)
add_subdirectory(qml/scene2d)
add_subdirectory(qml/scene3d)
add_subdirectory(qml/app)

target_link_libraries(KoggerApp PRIVATE
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,kogger_kqml_types,kogger_controls,kogger_menus,kogger_settings,kogger_devices,kogger_scene2d,kogger_scene3d,kogger_app>")

qt_add_translations(KoggerApp
    TS_FILES
        translations/translation_en.ts
        translations/translation_ru.ts
        translations/translation_pl.ts
    RESOURCE_PREFIX "/translations")

# ── Platform branches ────────────────────────────────────────────────────
if(ANDROID)
    set_target_properties(KoggerApp PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/platform/android
        QT_ANDROID_VERSION_NAME "${KOGGER_VERSION}"
        QT_ANDROID_VERSION_CODE "${KOGGER_VERSION_CODE}")
    target_link_options(KoggerApp PRIVATE $<$<CONFIG:Release>:-s>)
    target_include_directories(KoggerApp PRIVATE ${CMAKE_SOURCE_DIR})
    target_link_libraries(KoggerApp PRIVATE
        "${CMAKE_SOURCE_DIR}/third_party/freetype/lib/${CMAKE_ANDROID_ARCH_ABI}/libfreetype.a"
        z)
    target_sources(KoggerApp PRIVATE platform/android/shaders.qrc)
    if(NOT ANDROID_SDK_ROOT)
        set(ANDROID_SDK_ROOT "$ENV{ANDROID_SDK_ROOT}")
    endif()
    if(EXISTS "${ANDROID_SDK_ROOT}/android_openssl/android_openssl.cmake")
        include("${ANDROID_SDK_ROOT}/android_openssl/android_openssl.cmake")
        add_android_openssl_libraries(KoggerApp)
    endif()
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
