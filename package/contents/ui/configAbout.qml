import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.20 as Kirigami
import org.kde.plasma.jarvis 1.0

Item {
    id: aboutRootItem
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0

    ScrollView {
        id: aboutRoot
        anchors.fill: parent
        contentWidth: availableWidth

    ColumnLayout {
        width: aboutRoot.availableWidth
        spacing: Kirigami.Units.largeSpacing

        // ── HERO SECTION ──
        Kirigami.AbstractCard {
            Layout.fillWidth: true
            contentItem: ColumnLayout {
                spacing: Kirigami.Units.mediumSpacing

                RowLayout {
                    spacing: Kirigami.Units.largeSpacing
                    Layout.fillWidth: true

                    Kirigami.Icon {
                        source: "jarvis-ai"
                        implicitWidth: Kirigami.Units.iconSizes.huge
                        implicitHeight: Kirigami.Units.iconSizes.huge
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Kirigami.Heading {
                            text: "J.A.R.V.I.S."
                            level: 1
                        }
                        Label {
                            text: i18n("Just A Rather Very Intelligent System")
                            font.italic: true
                            color: Kirigami.Theme.disabledTextColor
                        }
                        Label {
                            text: i18n("Version %1", "0.1.0")
                            color: Kirigami.Theme.disabledTextColor
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                        }
                    }
                }

                Label {
                    text: i18n("A local AI assistant for KDE Plasma, inspired by Tony Stark's iconic AI. " +
                               "Powered entirely by open-source technology running on your machine — " +
                               "no cloud, no subscriptions, no data leaves your computer.")
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
            }
        }

        // ── AUTHOR ──
        Kirigami.Heading {
            text: i18n("Author")
            level: 3
        }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                RowLayout {
                    spacing: Kirigami.Units.largeSpacing
                    Layout.fillWidth: true

                    Kirigami.Icon {
                        source: "user-identity"
                        implicitWidth: Kirigami.Units.iconSizes.large
                        implicitHeight: Kirigami.Units.iconSizes.large
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            text: "Kamil 'Novik' Nowicki"
                            font.bold: true
                            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.1
                        }
                        Label {
                            text: i18n("Developer & Designer")
                            color: Kirigami.Theme.disabledTextColor
                        }
                    }
                }

                Kirigami.Separator { Layout.fillWidth: true }

                RowLayout {
                    spacing: Kirigami.Units.smallSpacing
                    Kirigami.Icon {
                        source: "mail-message"
                        implicitWidth: Kirigami.Units.iconSizes.small
                        implicitHeight: Kirigami.Units.iconSizes.small
                    }
                    Label {
                        text: "<a href=\"mailto:kamil@kamilnowicki.com\">kamil@kamilnowicki.com</a>"
                        onLinkActivated: function(link) { Qt.openUrlExternally(link) }
                    }
                }

                RowLayout {
                    spacing: Kirigami.Units.smallSpacing
                    Kirigami.Icon {
                        source: "internet-web-browser"
                        implicitWidth: Kirigami.Units.iconSizes.small
                        implicitHeight: Kirigami.Units.iconSizes.small
                    }
                    Label {
                        text: "<a href=\"https://kamilnowicki.com\">kamilnowicki.com</a>"
                        onLinkActivated: function(link) { Qt.openUrlExternally(link) }
                    }
                }

                RowLayout {
                    spacing: Kirigami.Units.smallSpacing
                    Kirigami.Icon {
                        source: "vcs-normal"
                        implicitWidth: Kirigami.Units.iconSizes.small
                        implicitHeight: Kirigami.Units.iconSizes.small
                    }
                    Label {
                        text: "<a href=\"https://github.com/novik133/jarvis\">github.com/novik133/jarvis</a>"
                        onLinkActivated: function(link) { Qt.openUrlExternally(link) }
                    }
                }

                Label {
                    text: "© 2026 Kamil Nowicki. " + i18n("Licensed under GPL-3.0.")
                    color: Kirigami.Theme.disabledTextColor
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                }
            }
        }

        // ── TECHNOLOGY ──
        Kirigami.Heading {
            text: i18n("Technology Stack")
            level: 3
        }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            contentItem: GridLayout {
                columns: 2
                columnSpacing: Kirigami.Units.largeSpacing
                rowSpacing: Kirigami.Units.smallSpacing

                Label { text: i18n("Framework:"); font.bold: true }
                Label { text: "KDE Plasma 6 · Qt 6 · C++23" }

                Label { text: i18n("AI Engine:"); font.bold: true }
                Label { text: "llama.cpp (local, OpenAI-compatible)" }

                Label { text: i18n("Speech Recognition:"); font.bold: true }
                Label { text: "whisper.cpp (CPU, offline)" }

                Label { text: i18n("Text-to-Speech:"); font.bold: true }
                Label { text: "Piper TTS · espeak-ng fallback" }

                Label { text: i18n("Audio Pipeline:"); font.bold: true }
                Label { text: "PipeWire / PulseAudio" }

                Label { text: i18n("Privacy:"); font.bold: true }
                Label {
                    text: i18n("100% local — no internet required for AI")
                    color: Kirigami.Theme.positiveTextColor
                    font.bold: true
                }
            }
        }

        // ── FEATURES ──
        Kirigami.Heading {
            text: i18n("Features")
            level: 3
        }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                Repeater {
                    model: [
                        { icon: "preferences-desktop-text-to-speech", text: i18n("Natural voice interaction with wake word detection") },
                        { icon: "dialog-messages", text: i18n("Conversational AI powered by local LLM") },
                        { icon: "audio-volume-high", text: i18n("High-quality text-to-speech with multiple voices") },
                        { icon: "utilities-system-monitor", text: i18n("Real-time system monitoring (CPU, RAM, temperature)") },
                        { icon: "dialog-scripts", text: i18n("Custom voice commands to control your system") },
                        { icon: "appointment-soon", text: i18n("Reminders and quick timers") },
                        { icon: "download", text: i18n("Download and manage LLM models and TTS voices") },
                        { icon: "system-lock-screen", text: i18n("Complete privacy — everything runs locally") }
                    ]
                    delegate: RowLayout {
                        spacing: Kirigami.Units.smallSpacing
                        Layout.fillWidth: true
                        Kirigami.Icon {
                            source: modelData.icon
                            implicitWidth: Kirigami.Units.iconSizes.smallMedium
                            implicitHeight: Kirigami.Units.iconSizes.smallMedium
                        }
                        Label {
                            text: modelData.text
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }
        }

        // ── SUPPORT ──
        Kirigami.Heading {
            text: i18n("Support the Project")
            level: 3
        }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            contentItem: ColumnLayout {
                spacing: Kirigami.Units.mediumSpacing

                Label {
                    text: i18n("J.A.R.V.I.S. is free and open source. If you find it useful, " +
                               "consider supporting its development. Every contribution helps " +
                               "keep the arc reactor running!")
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }

                RowLayout {
                    spacing: Kirigami.Units.largeSpacing
                    Layout.fillWidth: true

                    Button {
                        text: i18n("GitHub")
                        icon.name: "vcs-normal"
                        onClicked: Qt.openUrlExternally("https://github.com/novik133/jarvis")
                    }

                    Button {
                        text: i18n("Donate via PayPal")
                        icon.name: "help-donate"
                        onClicked: Qt.openUrlExternally("https://paypal.me/noviktech133")
                    }

                    Button {
                        text: i18n("Website")
                        icon.name: "internet-web-browser"
                        flat: true
                        onClicked: Qt.openUrlExternally("https://kamilnowicki.com")
                    }
                }
            }
        }

        // ── FOOTER ──
        Label {
            text: "\"At your service, Sir.\""
            font.italic: true
            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.1
            color: Kirigami.Theme.disabledTextColor
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
            Layout.bottomMargin: Kirigami.Units.largeSpacing
        }
    }
    }
}
