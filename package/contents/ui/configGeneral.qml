import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.20 as Kirigami
import org.kde.plasma.jarvis 1.0

Item {
    id: configRootItem
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0

    ScrollView {
        id: configRoot
        anchors.fill: parent
        contentWidth: availableWidth

    ColumnLayout {
        id: configPage
        width: configRoot.availableWidth
        spacing: 0

        // ════════════════════════════════════════
        // DOWNLOAD PROGRESS — always visible at top when downloading
        // ════════════════════════════════════════
        Kirigami.InlineMessage {
            id: downloadBanner
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
            visible: JarvisBackend.downloading
            type: Kirigami.MessageType.Information
            text: JarvisBackend.downloadStatus

            actions: [
                Kirigami.Action {
                    icon.name: "dialog-cancel"
                    text: i18n("Cancel")
                    onTriggered: JarvisBackend.cancelDownload()
                }
            ]
        }

        ProgressBar {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.smallSpacing
            Layout.rightMargin: Kirigami.Units.smallSpacing
            visible: JarvisBackend.downloading
            from: 0; to: 1.0
            value: JarvisBackend.downloadProgress
        }

        // ════════════════════════════════════════
        // DEPENDENCY STATUS
        // ════════════════════════════════════════
        Kirigami.FormLayout {
            Layout.fillWidth: true

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Component Status")
            }

            Label {
                text: i18n("All components are bundled with the plugin. No external installation required.")
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                color: Kirigami.Theme.disabledTextColor
                font.pointSize: Kirigami.Theme.smallFont.pointSize
            }

            RowLayout {
                Kirigami.FormData.label: i18n("LLM Engine:")
                spacing: Kirigami.Units.smallSpacing
                Kirigami.Icon {
                    source: JarvisBackend.llmServerBundled ? "emblem-default" : "emblem-warning"
                    implicitWidth: Kirigami.Units.iconSizes.small
                    implicitHeight: Kirigami.Units.iconSizes.small
                }
                Label {
                    text: JarvisBackend.llmServerBundled ? i18n("Bundled (llama.cpp)") : i18n("Not found")
                    color: JarvisBackend.llmServerBundled ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.negativeTextColor
                    font.bold: true
                }
            }

            RowLayout {
                Kirigami.FormData.label: i18n("Speech Recognition:")
                spacing: Kirigami.Units.smallSpacing
                Kirigami.Icon {
                    source: (JarvisBackend.currentWhisperModel !== "") ? "emblem-default" : "emblem-warning"
                    implicitWidth: Kirigami.Units.iconSizes.small
                    implicitHeight: Kirigami.Units.iconSizes.small
                }
                Label {
                    text: (JarvisBackend.currentWhisperModel !== "") ? i18n("Bundled (whisper.cpp) — Model: %1", JarvisBackend.currentWhisperModel) : i18n("No whisper model — download one below")
                    color: (JarvisBackend.currentWhisperModel !== "") ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.negativeTextColor
                    font.bold: true
                }
            }

            RowLayout {
                Kirigami.FormData.label: i18n("TTS Engine:")
                spacing: Kirigami.Units.smallSpacing
                Kirigami.Icon {
                    source: JarvisBackend.piperInstalled ? "emblem-default" : "emblem-warning"
                    implicitWidth: Kirigami.Units.iconSizes.small
                    implicitHeight: Kirigami.Units.iconSizes.small
                }
                Label {
                    text: JarvisBackend.piperInstalled ? i18n("Piper TTS installed") : i18n("Piper not found — download below")
                    color: JarvisBackend.piperInstalled ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.negativeTextColor
                    font.bold: true
                }
                Button {
                    visible: !JarvisBackend.piperInstalled
                    text: i18n("Download Piper")
                    icon.name: "download"
                    enabled: !JarvisBackend.downloading
                    onClicked: JarvisBackend.downloadPiperBinary()
                }
            }
        }

        // ════════════════════════════════════════
        // BUNDLED LLM SERVER
        // ════════════════════════════════════════
        Kirigami.FormLayout {
            Layout.fillWidth: true

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("LLM Server (Bundled llama.cpp)")
            }

            Label {
                text: i18n("The LLM server runs locally using the bundled llama.cpp. Download a model, then start the server.")
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                color: Kirigami.Theme.disabledTextColor
                font.pointSize: Kirigami.Theme.smallFont.pointSize
            }

            RowLayout {
                Kirigami.FormData.label: i18n("Server status:")
                spacing: Kirigami.Units.smallSpacing
                Kirigami.Icon {
                    source: JarvisBackend.llmServerRunning ? "media-playback-start" : "media-playback-stop"
                    implicitWidth: Kirigami.Units.iconSizes.small
                    implicitHeight: Kirigami.Units.iconSizes.small
                }
                Label {
                    text: JarvisBackend.llmServerRunning ? i18n("Running") : i18n("Stopped")
                    color: JarvisBackend.llmServerRunning ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.negativeTextColor
                    font.bold: true
                }
            }

            RowLayout {
                Kirigami.FormData.label: i18n("Server controls:")
                spacing: Kirigami.Units.smallSpacing
                Button {
                    text: JarvisBackend.llmServerRunning ? i18n("Restart Server") : i18n("Start Server")
                    icon.name: JarvisBackend.llmServerRunning ? "view-refresh" : "media-playback-start"
                    onClicked: {
                        if (JarvisBackend.llmServerRunning) JarvisBackend.restartLlmServer()
                        else JarvisBackend.startLlmServer()
                    }
                }
                Button {
                    text: i18n("Stop Server")
                    icon.name: "media-playback-stop"
                    enabled: JarvisBackend.llmServerRunning
                    onClicked: JarvisBackend.stopLlmServer()
                }
            }

            RowLayout {
                Kirigami.FormData.label: i18n("Server URL:")
                spacing: Kirigami.Units.smallSpacing
                TextField {
                    id: serverUrlField
                    text: JarvisBackend.llmServerUrl
                    placeholderText: "http://127.0.0.1:8080"
                    Layout.fillWidth: true
                    onAccepted: JarvisBackend.setLlmServerUrl(text)
                }
                Button {
                    text: i18n("Apply")
                    icon.name: "dialog-ok-apply"
                    onClicked: JarvisBackend.setLlmServerUrl(serverUrlField.text)
                }
            }

            RowLayout {
                Kirigami.FormData.label: i18n("Connection:")
                spacing: Kirigami.Units.smallSpacing
                Kirigami.Icon {
                    source: JarvisBackend.connected ? "network-connect" : "network-disconnect"
                    implicitWidth: Kirigami.Units.iconSizes.small
                    implicitHeight: Kirigami.Units.iconSizes.small
                }
                Label {
                    text: JarvisBackend.connected ? i18n("Connected") : i18n("Disconnected")
                    color: JarvisBackend.connected ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.negativeTextColor
                    font.bold: true
                }
            }

            Label {
                Kirigami.FormData.label: i18n("Active model:")
                text: JarvisBackend.currentModelName || i18n("None selected")
                font.bold: true
            }
        }

        // ════════════════════════════════════════
        // LLM MODELS
        // ════════════════════════════════════════
        Kirigami.FormLayout {
            Layout.fillWidth: true

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("LLM Models (GGUF for llama.cpp)")
            }

            Label {
                text: i18n("Select a model to use with your local LLM server. Smaller models are faster but less capable.")
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                color: Kirigami.Theme.disabledTextColor
                font.pointSize: Kirigami.Theme.smallFont.pointSize
            }
        }

        Repeater {
            model: JarvisBackend.availableLlmModels
            delegate: Kirigami.AbstractCard {
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.smallSpacing
                Layout.rightMargin: Kirigami.Units.smallSpacing
                contentItem: RowLayout {
                    spacing: Kirigami.Units.largeSpacing
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        RowLayout {
                            spacing: Kirigami.Units.smallSpacing
                            Label {
                                text: modelData.name
                                font.bold: true
                            }
                            Label {
                                text: modelData.size
                                color: Kirigami.Theme.disabledTextColor
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                            }
                            Kirigami.Icon {
                                visible: modelData.active
                                source: "emblem-default"
                                implicitWidth: Kirigami.Units.iconSizes.small
                                implicitHeight: Kirigami.Units.iconSizes.small
                            }
                        }
                        Label {
                            text: modelData.desc
                            color: Kirigami.Theme.disabledTextColor
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                        }
                    }
                    Button {
                        text: modelData.active ? i18n("Active") : (modelData.downloaded ? i18n("Activate") : i18n("Download"))
                        icon.name: modelData.active ? "checkmark" : (modelData.downloaded ? "media-playback-start" : "download")
                        enabled: !modelData.active && !JarvisBackend.downloading
                        highlighted: modelData.active
                        onClicked: {
                            if (modelData.downloaded) JarvisBackend.setActiveLlmModel(modelData.id)
                            else JarvisBackend.downloadLlmModel(modelData.id)
                        }
                    }
                }
            }
        }

        Button {
            text: i18n("Fetch More Models")
            icon.name: "list-add"
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.topMargin: Kirigami.Units.smallSpacing
            onClicked: JarvisBackend.fetchMoreModels()
        }

        // ════════════════════════════════════════
        // TTS VOICES
        // ════════════════════════════════════════
        Kirigami.FormLayout {
            Layout.fillWidth: true

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("TTS Voices (Piper)")
            }

            Label {
                text: i18n("Choose a voice for speech synthesis. Download a voice, then press Play to preview it.")
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                color: Kirigami.Theme.disabledTextColor
                font.pointSize: Kirigami.Theme.smallFont.pointSize
            }

            Label {
                Kirigami.FormData.label: i18n("Active voice:")
                text: JarvisBackend.currentVoiceName || i18n("None")
                font.bold: true
            }
        }

        Repeater {
            model: JarvisBackend.availableTtsVoices
            delegate: Kirigami.AbstractCard {
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.smallSpacing
                Layout.rightMargin: Kirigami.Units.smallSpacing
                contentItem: RowLayout {
                    spacing: Kirigami.Units.largeSpacing
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        RowLayout {
                            spacing: Kirigami.Units.smallSpacing
                            Label {
                                text: modelData.name
                                font.bold: true
                            }
                            Label {
                                text: modelData.lang
                                color: Kirigami.Theme.disabledTextColor
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                            }
                            Kirigami.Icon {
                                visible: modelData.active
                                source: "emblem-default"
                                implicitWidth: Kirigami.Units.iconSizes.small
                                implicitHeight: Kirigami.Units.iconSizes.small
                            }
                        }
                        Label {
                            text: modelData.desc
                            color: Kirigami.Theme.disabledTextColor
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                        }
                    }
                    Button {
                        visible: modelData.downloaded
                        text: i18n("Play")
                        icon.name: "media-playback-start"
                        flat: true
                        ToolTip.text: i18n("Preview this voice")
                        ToolTip.visible: hovered
                        onClicked: JarvisBackend.testVoice(modelData.id)
                    }
                    Button {
                        text: modelData.active ? i18n("Active") : (modelData.downloaded ? i18n("Activate") : i18n("Download"))
                        icon.name: modelData.active ? "checkmark" : (modelData.downloaded ? "media-playback-start" : "download")
                        enabled: !modelData.active && !JarvisBackend.downloading
                        highlighted: modelData.active
                        onClicked: {
                            if (modelData.downloaded) JarvisBackend.setActiveTtsVoice(modelData.id)
                            else JarvisBackend.downloadTtsVoice(modelData.id)
                        }
                    }
                }
            }
        }

        Button {
            text: i18n("Fetch More Voices")
            icon.name: "list-add"
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.topMargin: Kirigami.Units.smallSpacing
            onClicked: JarvisBackend.fetchMoreVoices()
        }

        // ════════════════════════════════════════
        // WHISPER MODELS (Speech Recognition)
        // ════════════════════════════════════════
        Kirigami.FormLayout {
            Layout.fillWidth: true

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Whisper Models (Speech Recognition)")
            }

            Label {
                text: i18n("Download a whisper model for wake word detection and voice commands. Smaller models are faster but less accurate.")
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                color: Kirigami.Theme.disabledTextColor
                font.pointSize: Kirigami.Theme.smallFont.pointSize
            }

            Label {
                Kirigami.FormData.label: i18n("Active model:")
                text: JarvisBackend.currentWhisperModel || i18n("None")
                font.bold: true
            }
        }

        Repeater {
            model: JarvisBackend.availableWhisperModels
            delegate: Kirigami.AbstractCard {
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.smallSpacing
                Layout.rightMargin: Kirigami.Units.smallSpacing
                contentItem: RowLayout {
                    spacing: Kirigami.Units.largeSpacing
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        RowLayout {
                            spacing: Kirigami.Units.smallSpacing
                            Label {
                                text: modelData.name
                                font.bold: true
                            }
                            Label {
                                text: modelData.size
                                color: Kirigami.Theme.disabledTextColor
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                            }
                            Kirigami.Icon {
                                visible: modelData.active
                                source: "emblem-default"
                                implicitWidth: Kirigami.Units.iconSizes.small
                                implicitHeight: Kirigami.Units.iconSizes.small
                            }
                        }
                        Label {
                            text: modelData.desc
                            color: Kirigami.Theme.disabledTextColor
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                        }
                    }
                    Button {
                        text: modelData.active ? i18n("Active") : (modelData.downloaded ? i18n("Activate") : i18n("Download"))
                        icon.name: modelData.active ? "checkmark" : (modelData.downloaded ? "media-playback-start" : "download")
                        enabled: !modelData.active && !JarvisBackend.downloading
                        highlighted: modelData.active
                        onClicked: {
                            if (modelData.downloaded) JarvisBackend.setActiveWhisperModel(modelData.id)
                            else JarvisBackend.downloadWhisperModel(modelData.id)
                        }
                    }
                }
            }
        }

        // ════════════════════════════════════════
        // PIPER TTS ENGINE
        // ════════════════════════════════════════
        Kirigami.FormLayout {
            Layout.fillWidth: true
            visible: !JarvisBackend.piperInstalled

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Piper TTS Engine")
            }

            Kirigami.InlineMessage {
                Layout.fillWidth: true
                visible: !JarvisBackend.piperInstalled
                type: Kirigami.MessageType.Warning
                text: i18n("Piper TTS is not installed. Download it to enable high-quality speech synthesis. Without Piper, the plugin falls back to espeak-ng.")

                actions: [
                    Kirigami.Action {
                        icon.name: "download"
                        text: i18n("Download Piper")
                        enabled: !JarvisBackend.downloading
                        onTriggered: JarvisBackend.downloadPiperBinary()
                    }
                ]
            }
        }

        // ════════════════════════════════════════
        // VOICE SYNTHESIS SETTINGS
        // ════════════════════════════════════════
        Kirigami.FormLayout {
            Layout.fillWidth: true

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Voice Synthesis Settings")
            }

            Slider {
                id: rateSlider
                Kirigami.FormData.label: i18n("Speech rate: %1", value.toFixed(2))
                from: -1.0; to: 1.0; stepSize: 0.05
                value: 0.05
                onMoved: JarvisBackend.setTtsRate(value)
            }

            Slider {
                id: pitchSlider
                Kirigami.FormData.label: i18n("Speech pitch: %1", value.toFixed(2))
                from: -1.0; to: 1.0; stepSize: 0.05
                value: -0.1
                onMoved: JarvisBackend.setTtsPitch(value)
            }

            Slider {
                id: volumeSlider
                Kirigami.FormData.label: i18n("Volume: %1%", (value * 100).toFixed(0))
                from: 0.0; to: 1.0; stepSize: 0.05
                value: 0.85
                onMoved: JarvisBackend.setTtsVolume(value)
            }

            Button {
                text: i18n("Test Current Voice")
                icon.name: "media-playback-start"
                onClicked: JarvisBackend.testVoice(JarvisBackend.currentVoiceName)
            }
        }

        // ════════════════════════════════════════
        // WAKE WORD & AUDIO
        // ════════════════════════════════════════
        Kirigami.FormLayout {
            Layout.fillWidth: true

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Wake Word & Audio")
            }

            CheckBox {
                Kirigami.FormData.label: i18n("Auto-start wake word detection:")
                checked: JarvisBackend.autoStartWakeWord
                onToggled: JarvisBackend.setAutoStartWakeWord(checked)
            }

            Label {
                text: i18n("Say \"Jarvis\" to activate voice commands without clicking.")
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                color: Kirigami.Theme.disabledTextColor
                font.pointSize: Kirigami.Theme.smallFont.pointSize
            }

            Slider {
                id: voiceCmdSlider
                Kirigami.FormData.label: i18n("Max voice command length: %1 seconds", value.toFixed(0))
                from: 3; to: 30; stepSize: 1
                value: JarvisBackend.voiceCmdMaxSeconds
                onMoved: JarvisBackend.setVoiceCmdMaxSeconds(value)
            }
        }

        // ════════════════════════════════════════
        // CHAT SETTINGS
        // ════════════════════════════════════════
        Kirigami.FormLayout {
            Layout.fillWidth: true

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Chat Settings")
            }

            Slider {
                id: historySlider
                Kirigami.FormData.label: i18n("Conversation memory: %1 message pairs", value.toFixed(0))
                from: 5; to: 100; stepSize: 5
                value: JarvisBackend.maxHistoryPairs
                onMoved: JarvisBackend.setMaxHistoryPairs(value)
            }

            Label {
                text: i18n("More pairs = better context memory but slower responses and more RAM usage.")
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                color: Kirigami.Theme.disabledTextColor
                font.pointSize: Kirigami.Theme.smallFont.pointSize
            }
        }

        // ════════════════════════════════════════
        // PERSONALITY
        // ════════════════════════════════════════
        Kirigami.FormLayout {
            Layout.fillWidth: true

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("AI Personality")
            }

            Label {
                text: i18n("Customize the system prompt to change how J.A.R.V.I.S. behaves. Leave empty for the default J.A.R.V.I.S. personality.")
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                color: Kirigami.Theme.disabledTextColor
                font.pointSize: Kirigami.Theme.smallFont.pointSize
            }

            TextArea {
                id: personalityField
                Kirigami.FormData.label: i18n("System prompt:")
                text: JarvisBackend.personalityPrompt
                placeholderText: i18n("Default: J.A.R.V.I.S. from Iron Man — polite, witty, British humor...")
                Layout.fillWidth: true
                Layout.preferredHeight: 120
                wrapMode: TextEdit.Wrap
            }

            Button {
                text: i18n("Save Personality")
                icon.name: "document-save"
                onClicked: JarvisBackend.setPersonalityPrompt(personalityField.text)
            }
        }

        // Bottom spacer
        Item { Layout.preferredHeight: Kirigami.Units.largeSpacing }
    }
    }
}
