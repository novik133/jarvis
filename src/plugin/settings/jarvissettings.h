#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class JarvisSettings : public QObject
{
    Q_OBJECT

public:
    explicit JarvisSettings(QNetworkAccessManager *nam, QObject *parent = nullptr);

    // Getters
    [[nodiscard]] QString llmServerUrl() const { return m_llmServerUrl; }
    [[nodiscard]] QString currentModelName() const { return m_currentModelName; }
    [[nodiscard]] QString currentVoiceName() const { return m_currentVoiceName; }
    [[nodiscard]] QVariantList availableLlmModels() const { return m_availableLlmModels; }
    [[nodiscard]] QVariantList availableTtsVoices() const { return m_availableTtsVoices; }
    [[nodiscard]] double downloadProgress() const { return m_downloadProgress; }
    [[nodiscard]] bool isDownloading() const { return m_downloading; }
    [[nodiscard]] QString downloadStatus() const { return m_downloadStatus; }
    [[nodiscard]] int maxHistoryPairs() const { return m_maxHistoryPairs; }
    [[nodiscard]] int wakeBufferSeconds() const { return m_wakeBufferSeconds; }
    [[nodiscard]] int voiceCmdMaxSeconds() const { return m_voiceCmdMaxSeconds; }
    [[nodiscard]] bool autoStartWakeWord() const { return m_autoStartWakeWord; }
    [[nodiscard]] QString personalityPrompt() const { return m_personalityPrompt; }
    [[nodiscard]] double ttsRate() const { return m_ttsRate; }
    [[nodiscard]] double ttsPitch() const { return m_ttsPitch; }
    [[nodiscard]] double ttsVolume() const { return m_ttsVolume; }
    [[nodiscard]] bool ttsMuted() const { return m_ttsMuted; }

    // Setters
    void setLlmServerUrl(const QString &url);
    void setCurrentModelName(const QString &name);
    void setMaxHistoryPairs(int pairs);
    void setWakeBufferSeconds(int seconds);
    void setVoiceCmdMaxSeconds(int seconds);
    void setAutoStartWakeWord(bool enabled);
    void setPersonalityPrompt(const QString &prompt);
    void setTtsRate(double rate);
    void setTtsPitch(double pitch);
    void setTtsVolume(double volume);
    void setTtsMuted(bool muted);

    // Downloads
    void downloadLlmModel(const QString &modelId);
    void downloadTtsVoice(const QString &voiceId);
    void setActiveLlmModel(const QString &modelId);
    void setActiveTtsVoice(const QString &voiceId);
    void cancelDownload();

    // Helpers
    [[nodiscard]] QString jarvisDataDir() const;
    [[nodiscard]] QString piperModelPath() const { return m_piperModelPath; }
    void populateModelList();
    void populateVoiceList();
    void fetchMoreModels();
    void fetchMoreVoices();

signals:
    void llmServerUrlChanged();
    void currentModelNameChanged();
    void currentVoiceNameChanged();
    void downloadProgressChanged();
    void downloadingChanged();
    void downloadStatusChanged();
    void maxHistoryPairsChanged();
    void wakeBufferSecondsChanged();
    void voiceCmdMaxSecondsChanged();
    void autoStartWakeWordChanged();
    void personalityPromptChanged();
    void ttsRateChanged();
    void ttsPitchChanged();
    void ttsVolumeChanged();
    void ttsMutedChanged();
    void voiceActivated(const QString &voiceId, const QString &onnxPath);

private:
    void loadSettings();
    void saveSettings();

    QSettings m_settings{QStringLiteral("jarvis-plasmoid"), QStringLiteral("jarvis")};
    QNetworkAccessManager *m_networkManager{nullptr};

    QString m_llmServerUrl{QStringLiteral("http://127.0.0.1:8080")};
    QString m_currentModelName;
    QString m_currentVoiceName;
    QVariantList m_availableLlmModels;
    QVariantList m_availableTtsVoices;
    double m_downloadProgress{0.0};
    bool m_downloading{false};
    QString m_downloadStatus;
    QNetworkReply *m_downloadReply{nullptr};
    int m_maxHistoryPairs{20};
    int m_wakeBufferSeconds{2};
    int m_voiceCmdMaxSeconds{8};
    bool m_autoStartWakeWord{true};
    QString m_personalityPrompt;
    double m_ttsRate{0.05};
    double m_ttsPitch{-0.1};
    double m_ttsVolume{0.85};
    bool m_ttsMuted{false};
    QString m_piperModelPath;
};
