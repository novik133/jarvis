set(jarvis_plugin_SRCS
    jarvisplugin.cpp
    jarvisbackend.cpp
    settings/jarvissettings.cpp
    tts/jarvisTts.cpp
    audio/jarvisaudio.cpp
    system/jarvissystem.cpp
    commands/jarviscommands.cpp
)

add_library(jarvis_plugin SHARED ${jarvis_plugin_SRCS})

target_include_directories(jarvis_plugin PRIVATE
    ${WHISPER_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(jarvis_plugin
    Qt6::Quick
    Qt6::Qml
    Qt6::Network
    Qt6::TextToSpeech
    Qt6::Multimedia
    Qt6::Concurrent
    Plasma::Plasma
    KF6::I18n
    ${WHISPER_LIBRARIES}
)

target_compile_features(jarvis_plugin PUBLIC cxx_std_23)

install(TARGETS jarvis_plugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/jarvis)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/jarvis)
