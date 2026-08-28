# ── RTSP video: libavformat/libavcodec from the Qt installation ──────────────
# Rationale, provenance and the header trap are documented in docs/, not here.

target_sources(KoggerApp PRIVATE
    video_stream.h            video_stream.cpp
    video_stream_pool.h       video_stream_pool.cpp)

target_include_directories(KoggerApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set(FFMPEG_INCLUDE_DIR "" CACHE PATH "Directory containing libavformat/ libavcodec/ headers")

if(WIN32)
    set(_ff_libdir "${QT6_INSTALL_PREFIX}/bin")
    set(_ff_libs avformat-61 avcodec-61 avutil-59 swscale-8)
    set(_ff_suffix ".dll")
else()
    set(_ff_libdir "${QT6_INSTALL_PREFIX}/lib")
    set(_ff_libs libavformat libavcodec libavutil libswscale)
    set(_ff_suffix ".so")
endif()

if(NOT FFMPEG_INCLUDE_DIR)
    set(FFMPEG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/ffmpeg/include")
endif()

if(NOT EXISTS "${FFMPEG_INCLUDE_DIR}/libavformat/avformat.h")
    message(FATAL_ERROR
        "FFmpeg headers not found at ${FFMPEG_INCLUDE_DIR}. Qt ships the FFmpeg "
        "libraries but not their headers; the matching tree is expected under "
        "third_party/ffmpeg/include. Override with "
        "-DFFMPEG_INCLUDE_DIR=<dir containing libavformat/avformat.h>")
endif()

set(_ff_link "")
foreach(_lib IN LISTS _ff_libs)
    set(_path "${_ff_libdir}/${_lib}${_ff_suffix}")
    if(NOT EXISTS "${_path}")
        message(FATAL_ERROR
            "FFmpeg library ${_path} not found in the Qt installation. Qt was probably "
            "built against a different FFmpeg major version - check the actual names in "
            "${_ff_libdir} and update _ff_libs.")
    endif()
    list(APPEND _ff_link "${_path}")
endforeach()

target_include_directories(KoggerApp SYSTEM PRIVATE "${FFMPEG_INCLUDE_DIR}")
target_link_libraries(KoggerApp PRIVATE ${_ff_link})
