cmake_minimum_required(VERSION 3.8)

project(digitalstage LANGUAGES CXX)

# Set version number.
set(digitalstage_VERSION_MAJOR 0)
set(digitalstage_VERSION_MINOR 1)

# C++ standard requirements.
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add some compile flags to our source files.
set_source_files_properties(${SOURCE_FILES}
        PROPERTIES COMPILE_FLAGS -Wall -Wextra -Wpedantic)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})

target_sources(${PROJECT_NAME} PRIVATE
        src_cpp/Broadcaster.cpp
        src_cpp/DigitalStageClient.cpp
        src_cpp/addon.cc
        src_cpp/MediaStreamTrackFactory.cpp
        src_cpp/RtAudioCaptureModule.cpp
        src_cpp/audio_device_rtaudio.cc
        )

# Extra libs required in order to use Apple media devices.
if(APPLE)
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
endif(APPLE)

# Source Dependencies.
add_subdirectory(deps/cpr "${CMAKE_CURRENT_BINARY_DIR}/cpr")
add_subdirectory(deps/libmediasoupclient "${CMAKE_CURRENT_BINARY_DIR}/libmediasoupclient")
add_subdirectory(deps/libwebrtc "${CMAKE_CURRENT_BINARY_DIR}/libwebrtc")

target_include_directories(${PROJECT_NAME}
        PRIVATE ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_JS_INC})

target_include_directories(${PROJECT_NAME}
        PUBLIC ${CPR_INCLUDE_DIRS}
        PUBLIC "${PROJECT_SOURCE_DIR}/deps/libmediasoupclient/include"
        PUBLIC "${PROJECT_SOURCE_DIR}/deps/libwebrtc")

# Include N-API wrappers
execute_process(COMMAND node -p "require('node-addon-api').include"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE NODE_ADDON_API_DIR
        )
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${NODE_ADDON_API_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(${PROJECT_NAME} PUBLIC
        mediasoupclient
        webrtc_broadcaster
        ${CPR_LIBRARIES}
        ${CMAKE_JS_LIB}
        )
