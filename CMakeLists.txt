cmake_minimum_required(VERSION 3.9)
cmake_policy(SET CMP0042 NEW)
set(digitalstage_VERSION_MAJOR 0)
set(digitalstage_VERSION_MINOR 1)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(digitalstage)
include_directories(${CMAKE_JS_INC} PRIVATE ${PROJECT_SOURCE_DIR}/include)

add_subdirectory(deps/libmediasoupclient "${CMAKE_CURRENT_BINARY_DIR}/libmediasoupclient")
add_subdirectory(deps/cpr "${CMAKE_CURRENT_BINARY_DIR}/cpr")

file(GLOB SOURCE_FILES src_cpp/addon.cc
        src_cpp/Broadcaster.cpp
        src_cpp/MediaStreamTrackFactory.cpp
        src_cpp/fakegenerators/fake_audio_capture_module.cc
        src_cpp/fakegenerators/fake_frame_source.cc
        src_cpp/fakegenerators/frame_generator_capturer.cc
        src_cpp/fakegenerators/frame_generator.cc
        src_cpp/fakegenerators/test_video_capturer.cc
        src_cpp/fakegenerators/frame_utils.cc
        src_cpp/fakegenerators/task_queue_for_test.cc)
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})

set_source_files_properties(${SOURCE_FILES}
        PROPERTIES COMPILE_FLAGS -Wall -Wextra -Wpedantic)

# Extra libs required in order to use Apple media devices.
if (APPLE)
    find_library(APPLICATION_SERVICES ApplicationServices)
    find_library(AUDIO_TOOLBOX AudioToolbox)
    find_library(AV_FOUNDATION AVFoundation)
    find_library(CORE_AUDIO CoreAudio)
    find_library(CORE_FOUNDATION Foundation)
    find_library(CORE_MEDIA CoreMedia)
    find_library(CORE_VIDEO CoreVideo)
    find_library(CORE_SERVICES CoreServices)

    target_link_libraries(${PROJECT_NAME} PUBLIC
            ${APPLICATION_SERVICES}
            ${AUDIO_TOOLBOX}
            ${AV_FOUNDATION}
            ${CORE_AUDIO}
            ${CORE_FOUNDATION}
            ${CORE_MEDIA}
            ${CORE_VIDEO}
            ${CORE_SERVICES}
            )
endif (APPLE)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(${PROJECT_NAME} PUBLIC
        mediasoupclient
        ${CPR_LIBRARIES}
        ${CMAKE_JS_LIB}
        )

# Include N-API wrappers
execute_process(COMMAND node -p "require('node-addon-api').include"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE NODE_ADDON_API_DIR
        )
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/deps/libmediasoupclient/include"
        ${CPR_INCLUDE_DIRS} PRIVATE ${NODE_ADDON_API_DIR})

# define NPI_VERSION
add_definitions(-DNAPI_VERSION=3)