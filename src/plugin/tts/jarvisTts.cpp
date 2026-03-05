#include "jarvisTts.h"
#include "../settings/jarvissettings.h"

#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QRegularExpression>
#include <QLocale>
#include <QVoice>
#include <QDebug>

JarvisTts::JarvisTts(JarvisSettings *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
    initTts();
}

void JarvisTts::initTts()
{
    const QStringList piperPaths = {
        QStringLiteral("/usr/lib/piper-tts/bin/piper"),
        QStringLiteral("/usr/bin/piper"),
        QStringLiteral("/usr/local/bin/piper"),
    };

    QString piperBin;
    for (const auto &path : piperPaths) {
        if (QFile::exists(path)) { piperBin = path; break; }
    }

    const QString modelPath = m_settings->piperModelPath();

    if (!piperBin.isEmpty() && !modelPath.isEmpty()) {
        m_usePiper = true;
        m_piperBin = piperBin;
        qDebug() << "[JARVIS] Using piper-tts for speech:" << piperBin << "model:" << modelPath;
    } else {
        m_usePiper = false;
        qDebug() << "[JARVIS] Piper not found, falling back to espeak-ng";

        m_tts = new QTextToSpeech(this);
        const auto voices = m_tts->availableVoices();
        for (const auto &voice : voices) {
            if (voice.locale().language() == QLocale::English &&
                voice.gender() == QVoice::Male) {
                m_tts->setVoice(voice);
                break;
            }
        }
        m_tts->setRate(m_settings->ttsRate());
        m_tts->setPitch(m_settings->ttsPitch());
        m_tts->setVolume(m_settings->ttsVolume());

        connect(m_tts, &QTextToSpeech::stateChanged, this, [this](QTextToSpeech::State state) {
            const bool wasSpeaking = m_speaking.load();
            m_speaking = (state == QTextToSpeech::Speaking);
            if (wasSpeaking != m_speaking.load()) {
                emit speakingChanged();
            }
        });
    }
}

void JarvisTts::speak(const QString &text)
{
    if (m_settings->ttsMuted()) return;

    QString cleanText = text;
    cleanText.remove(QRegularExpression(QStringLiteral("[*_`#]")));
    cleanText.replace(QStringLiteral("\n"), QStringLiteral(". "));

    if (m_usePiper) {
        if (m_piperProcess && m_piperProcess->state() != QProcess::NotRunning) {
            m_piperProcess->kill();
            m_piperProcess->waitForFinished(1000);
        }
        delete m_piperProcess;
        m_piperProcess = nullptr;

        const QString wavPath = QDir::tempPath() + QStringLiteral("/jarvis_tts_%1.wav").arg(QDateTime::currentMSecsSinceEpoch());

        const double lengthScale = 1.0 - (m_settings->ttsRate() * 0.5);

        if (m_piperBin.isEmpty()) { qWarning() << "[JARVIS] Piper binary not found"; return; }

        m_piperProcess = new QProcess(this);

        const QString quotedText = QStringLiteral("'") + cleanText.replace(QStringLiteral("'"), QStringLiteral("'\\''")) + QStringLiteral("'");

        QString cmd;
        cmd += QStringLiteral("echo ") + quotedText;
        cmd += QStringLiteral(" | ") + m_piperBin;
        cmd += QStringLiteral(" -m ") + m_settings->piperModelPath();
        cmd += QStringLiteral(" -f ") + wavPath;
        cmd += QStringLiteral(" --length-scale ") + QString::number(lengthScale, 'f', 2);
        cmd += QStringLiteral(" --sentence-silence 0.3");
        cmd += QStringLiteral(" --volume ") + QString::number(m_settings->ttsVolume(), 'f', 2);
        cmd += QStringLiteral(" 2>/dev/null && pw-play ") + wavPath;
        cmd += QStringLiteral(" 2>/dev/null; rm -f ") + wavPath;

        qDebug() << "[JARVIS] TTS cmd:" << cmd;

        connect(m_piperProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this](int, QProcess::ExitStatus) {
            m_speaking = false;
            emit speakingChanged();
        });

        m_piperProcess->start(QStringLiteral("/bin/sh"), {QStringLiteral("-c"), cmd});

        m_speaking = true;
        emit speakingChanged();
    } else if (m_tts) {
        m_tts->say(cleanText);
    }
}

void JarvisTts::stop()
{
    if (m_usePiper) {
        if (m_piperProcess && m_piperProcess->state() != QProcess::NotRunning) {
            m_piperProcess->kill();
            m_piperProcess->waitForFinished(500);
        }
        m_speaking = false;
        emit speakingChanged();
    } else if (m_tts) {
        m_tts->stop();
    }
}

void JarvisTts::toggleMute()
{
    m_settings->setTtsMuted(!m_settings->ttsMuted());
    if (m_settings->ttsMuted() && m_tts) {
        m_tts->stop();
    }
}

bool JarvisTts::isMuted() const
{
    return m_settings->ttsMuted();
}

void JarvisTts::onTtsRateChanged()
{
    if (!m_usePiper && m_tts) m_tts->setRate(m_settings->ttsRate());
}

void JarvisTts::onTtsPitchChanged()
{
    if (!m_usePiper && m_tts) m_tts->setPitch(m_settings->ttsPitch());
}

void JarvisTts::onTtsVolumeChanged()
{
    if (!m_usePiper && m_tts) {
        m_tts->setVolume(m_settings->ttsVolume());
    }
    // Piper uses --volume flag at speak time, no runtime change needed
}

void JarvisTts::onVoiceActivated(const QString &voiceId, const QString &onnxPath)
{
    Q_UNUSED(voiceId)
    Q_UNUSED(onnxPath)
    // Piper uses the model path from settings at speak time, no reinit needed
}
