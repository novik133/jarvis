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

JarvisTts::~JarvisTts()
{
    stop();
    stopPiperProcess();
}

void JarvisTts::initTts()
{
    // Use settings-managed Piper binary path (bundled or system)
    const QString settingsBin = m_settings->piperBinaryPath();
    if (!settingsBin.isEmpty() && QFile::exists(settingsBin)) {
        m_piperBin = settingsBin;
    } else {
        // Fallback search
        const QStringList piperPaths = {
            m_settings->jarvisDataDir() + QStringLiteral("/piper/piper"),
            QStringLiteral("/usr/lib/piper-tts/bin/piper"),
            QStringLiteral("/usr/bin/piper"),
            QStringLiteral("/usr/local/bin/piper"),
        };

        for (const auto &path : piperPaths) {
            if (QFile::exists(path)) { m_piperBin = path; break; }
        }
    }

    const QString modelPath = m_settings->piperModelPath();

    if (!m_piperBin.isEmpty() && !modelPath.isEmpty()) {
        m_usePiper = true;
        qDebug() << "[JARVIS] Using piper-tts for speech:" << m_piperBin << "model:" << modelPath;
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

// ─────────────────────────────────────────────
// Persistent Piper Process Management
// ─────────────────────────────────────────────

void JarvisTts::startPiperProcess()
{
    if (m_piperProcess && m_piperProcess->state() == QProcess::Running) {
        return; // Already running
    }

    stopPiperProcess();

    m_currentWavPath = QDir::tempPath() + QStringLiteral("/jarvis_tts_pipe.wav");

    // Piper reads JSON-lines from stdin when using --json-input and --output-file
    // For persistent mode: we pipe text lines to stdin, piper writes wav to file
    // We use a wrapper script that reads lines and synthesizes each one
    m_piperProcess = new QProcess(this);
    m_piperProcess->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_piperProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &JarvisTts::onPiperFinished);

    qDebug() << "[JARVIS] Starting persistent Piper process";
}

void JarvisTts::stopPiperProcess()
{
    if (m_piperProcess) {
        if (m_piperProcess->state() != QProcess::NotRunning) {
            m_piperProcess->kill();
            m_piperProcess->waitForFinished(1000);
        }
        delete m_piperProcess;
        m_piperProcess = nullptr;
    }
    if (m_playProcess) {
        if (m_playProcess->state() != QProcess::NotRunning) {
            m_playProcess->kill();
            m_playProcess->waitForFinished(500);
        }
        delete m_playProcess;
        m_playProcess = nullptr;
    }
}

void JarvisTts::onPiperFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(status)
    if (exitCode != 0) {
        qWarning() << "[JARVIS] Piper process exited with code:" << exitCode;
    }
}

// ─────────────────────────────────────────────
// Sentence Splitting
// ─────────────────────────────────────────────

QStringList JarvisTts::splitIntoSentences(const QString &text)
{
    QStringList sentences;
    // Split on sentence-ending punctuation followed by whitespace or end
    static const QRegularExpression sentenceRe(
        QStringLiteral("(?<=[.!?;:])\\s+|(?<=[.!?;:])$"));

    const auto parts = text.split(sentenceRe, Qt::SkipEmptyParts);
    for (const auto &part : parts) {
        const QString trimmed = part.trimmed();
        if (!trimmed.isEmpty()) {
            sentences.append(trimmed);
        }
    }

    // If no punctuation found, return the whole text as one sentence
    if (sentences.isEmpty() && !text.trimmed().isEmpty()) {
        sentences.append(text.trimmed());
    }

    return sentences;
}

// ─────────────────────────────────────────────
// Speak (full text or single sentence)
// ─────────────────────────────────────────────

void JarvisTts::speak(const QString &text)
{
    if (m_settings->ttsMuted()) return;

    QString cleanText = text;
    cleanText.remove(QRegularExpression(QStringLiteral("[*_`#]")));
    cleanText.replace(QStringLiteral("\n"), QStringLiteral(". "));

    if (m_usePiper) {
        // Split into sentences and queue them for streaming playback
        const QStringList sentences = splitIntoSentences(cleanText);
        {
            QMutexLocker lock(&m_queueMutex);
            for (const auto &s : sentences) {
                m_sentenceQueue.enqueue(s);
            }
        }
        if (!m_playingBack) {
            processNextSentence();
        }
    } else if (m_tts) {
        m_tts->say(cleanText);
    }
}

void JarvisTts::speakSentence(const QString &sentence)
{
    if (m_settings->ttsMuted()) return;
    if (sentence.trimmed().isEmpty()) return;

    QString cleanText = sentence;
    cleanText.remove(QRegularExpression(QStringLiteral("[*_`#]")));
    cleanText.replace(QStringLiteral("\n"), QStringLiteral(". "));

    if (m_usePiper) {
        {
            QMutexLocker lock(&m_queueMutex);
            m_sentenceQueue.enqueue(cleanText);
        }
        if (!m_playingBack) {
            processNextSentence();
        }
    } else if (m_tts) {
        m_tts->say(cleanText);
    }
}

void JarvisTts::processNextSentence()
{
    QString sentence;
    {
        QMutexLocker lock(&m_queueMutex);
        if (m_sentenceQueue.isEmpty()) {
            m_playingBack = false;
            m_speaking = false;
            emit speakingChanged();
            return;
        }
        sentence = m_sentenceQueue.dequeue();
    }

    m_playingBack = true;
    if (!m_speaking.load()) {
        m_speaking = true;
        emit speakingChanged();
    }

    if (m_piperBin.isEmpty()) {
        qWarning() << "[JARVIS] Piper binary not found";
        m_playingBack = false;
        m_speaking = false;
        emit speakingChanged();
        return;
    }

    // Synthesize this sentence to a temp wav, then play it
    const QString wavPath = QDir::tempPath() +
        QStringLiteral("/jarvis_tts_%1.wav").arg(QDateTime::currentMSecsSinceEpoch());

    const double lengthScale = 1.0 - (m_settings->ttsRate() * 0.5);

    const QString quotedText = QStringLiteral("'") +
        QString(sentence).replace(QStringLiteral("'"), QStringLiteral("'\\''")) +
        QStringLiteral("'");

    // Synthesize: pipe text to piper, output wav
    QString cmd;
    cmd += QStringLiteral("printf '%s' ") + quotedText;
    cmd += QStringLiteral(" | '") + m_piperBin + QStringLiteral("'");
    cmd += QStringLiteral(" -m '") + m_settings->piperModelPath() + QStringLiteral("'");
    cmd += QStringLiteral(" -f '") + wavPath + QStringLiteral("'");
    cmd += QStringLiteral(" --length-scale ") + QString::number(lengthScale, 'f', 2);
    cmd += QStringLiteral(" --sentence-silence 0.2");
    cmd += QStringLiteral(" 2>/dev/null");

    // Use a single shell process: synthesize then play then cleanup, then signal done
    cmd += QStringLiteral(" && pw-play '") + wavPath + QStringLiteral("' 2>/dev/null");
    cmd += QStringLiteral("; rm -f '") + wavPath + QStringLiteral("'");

    auto *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc](int, QProcess::ExitStatus) {
        proc->deleteLater();
        // Play next sentence in queue
        processNextSentence();
    });

    proc->start(QStringLiteral("/bin/sh"), {QStringLiteral("-c"), cmd});
}

void JarvisTts::playWavFile(const QString &wavPath)
{
    Q_UNUSED(wavPath)
    // Playback is handled inside processNextSentence shell command
}

void JarvisTts::stop()
{
    // Clear the sentence queue
    {
        QMutexLocker lock(&m_queueMutex);
        m_sentenceQueue.clear();
    }
    m_playingBack = false;

    if (m_usePiper) {
        stopPiperProcess();
        // Also kill any in-flight synthesis/playback processes
        // They are children that will be cleaned up
    }
    if (m_tts) {
        m_tts->stop();
    }

    if (m_speaking.load()) {
        m_speaking = false;
        emit speakingChanged();
    }
}

void JarvisTts::toggleMute()
{
    m_settings->setTtsMuted(!m_settings->ttsMuted());
    if (m_settings->ttsMuted()) {
        stop();
    }
}

bool JarvisTts::isMuted() const
{
    return m_settings->ttsMuted();
}

void JarvisTts::onTtsRateChanged()
{
    if (!m_usePiper && m_tts) m_tts->setRate(m_settings->ttsRate());
    // Piper uses --length-scale at speak time
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
    // Piper volume is controlled by pw-play's default volume
}

void JarvisTts::onVoiceActivated(const QString &voiceId, const QString &onnxPath)
{
    Q_UNUSED(voiceId)
    Q_UNUSED(onnxPath)
    // Piper model path is read from settings at speak time
}
