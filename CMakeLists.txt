cmake_minimum_required(VERSION 3.25)

project(jarvis-plasmoid VERSION 0.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Native architecture optimizations
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -O3")

set(QT_DEFAULT_MAJOR_VERSION 6)
set(CMAKE_PROJECT_VERSION "0.2.0")

find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)

find_package(Qt6 REQUIRED COMPONENTS
    Quick
    Qml
    Network
    TextToSpeech
    Multimedia
    Concurrent
)

find_package(KF6 REQUIRED COMPONENTS
    I18n
    Config
)

find_package(Plasma REQUIRED)

# ─────────────────────────────────────────────
# Bundled whisper.cpp (built from source)
# ─────────────────────────────────────────────
include(FetchContent)

FetchContent_Declare(
    whisper
    GIT_REPOSITORY https://github.com/ggerganov/whisper.cpp.git
    GIT_TAG        v1.7.3
    GIT_SHALLOW    TRUE
)

set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(whisper)

set(WHISPER_LIBRARIES whisper)
set(WHISPER_INCLUDE_DIRS ${whisper_SOURCE_DIR}/include ${whisper_SOURCE_DIR})
message(STATUS "Bundled whisper.cpp from: ${whisper_SOURCE_DIR}")

# ─────────────────────────────────────────────
# Build and bundle llama.cpp from source
# Automatically builds during package compilation
# ─────────────────────────────────────────────
# Use ExecuteProcess to build llama.cpp separately
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/BuildLlama.cmake.in
    ${CMAKE_BINARY_DIR}/build_llama.cmake
    @ONLY
)

# Custom command to build llama-server
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/llama-server
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/build_llama.cmake
    COMMENT "Building llama.cpp from source"
)

# Custom target to ensure llama-server is built
add_custom_target(
    build-llama-server ALL
    DEPENDS ${CMAKE_BINARY_DIR}/llama-server
)

# Install the built llama-server
install(PROGRAMS ${CMAKE_BINARY_DIR}/llama-server DESTINATION libexec/jarvis)

message(STATUS "Will build llama.cpp from source during compilation")

# Install the plasmoid package
plasma_install_package(package org.kde.plasma.jarvis)

# Install icons into hicolor theme
ecm_install_icons(
    ICONS
        icons/16-apps-jarvis-ai.png
        icons/22-apps-jarvis-ai.png
        icons/32-apps-jarvis-ai.png
        icons/48-apps-jarvis-ai.png
        icons/64-apps-jarvis-ai.png
        icons/128-apps-jarvis-ai.png
        icons/256-apps-jarvis-ai.png
        icons/sc-apps-jarvis-ai.svg
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)

add_subdirectory(src/plugin)
