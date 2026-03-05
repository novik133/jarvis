cmake_minimum_required(VERSION 3.25)

project(jarvis-plasmoid VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_DEFAULT_MAJOR_VERSION 6)
set(QT_MAJOR_VERSION 6)

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

# whisper.cpp - lightweight local speech recognition
find_library(WHISPER_LIB NAMES whisper PATHS /usr/local/lib /usr/lib)
find_path(WHISPER_INC whisper.h PATHS /usr/local/include /usr/include)
if(NOT WHISPER_LIB OR NOT WHISPER_INC)
    message(FATAL_ERROR "whisper.cpp not found. Install it: https://github.com/ggerganov/whisper.cpp")
endif()
set(WHISPER_LIBRARIES ${WHISPER_LIB})
set(WHISPER_INCLUDE_DIRS ${WHISPER_INC})
message(STATUS "Found whisper: ${WHISPER_LIB}")

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
