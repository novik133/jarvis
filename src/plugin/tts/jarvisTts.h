#pragma once

#include <QObject>
#include <QString>
#include <QProcess>
#include <QTextToSpeech>
#include <atomic>

class JarvisSettings;

class JarvisTts : public QObject
{
    Q_OBJECT

public:
    explicit JarvisTts(JarvisSettings *settings, QObject *parent = nullptr);

    [[nodiscard]] bool isSpeaking() const { return m_speaking.load(); }
    [[nodiscard]] bool isMuted() const;

    void speak(const QString &text);
    void stop();
    void toggleMute();

    // Called when settings change
    void onTtsRateChanged();
    void onTtsPitchChanged();
    void onTtsVolumeChanged();
    void onVoiceActivated(const QString &voiceId, const QString &onnxPath);

signals:
    void speakingChanged();

private:
    void initTts();

    JarvisSettings *m_settings{nullptr};

    QTextToSpeech *m_tts{nullptr};
    QProcess *m_piperProcess{nullptr};
    QString m_piperBin;
    bool m_usePiper{false};
    std::atomic<bool> m_speaking{false};
};
