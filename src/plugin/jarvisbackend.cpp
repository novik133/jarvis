#include "jarvisbackend.h"
#include "settings/jarvissettings.h"
#include "tts/jarvisTts.h"
#include "audio/jarvisaudio.h"
#include "system/jarvissystem.h"
#include "commands/jarviscommands.h"
#include "llm/jarvisllmmanager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QDebug>

// ─────────────────────────────────────────────
// Construction / Destruction
// ─────────────────────────────────────────────

JarvisBackend::JarvisBackend(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_healthCheckTimer(new QTimer(this))
    , m_reminderTimer(new QTimer(this))
{
    // Create modules
    m_settings = new JarvisSettings(m_networkManager, this);
    m_tts = new JarvisTts(m_settings, this);
    m_audio = new JarvisAudio(m_settings, this);
    m_system = new JarvisSystem(this);
    m_commands = new JarvisCommands(this);
    m_llmManager = new JarvisLlmManager(m_settings, this);

    connectModuleSignals();

    // Health check — every 10s
    connect(m_healthCheckTimer, &QTimer::timeout, this, &JarvisBackend::checkConnection);
    m_healthCheckTimer->start(10000);
    checkConnection();

    // Reminder check — every 1s
    connect(m_reminderTimer, &QTimer::timeout, this, &JarvisBackend::checkReminders);
    m_reminderTimer->start(1000);

    setStatus("J.A.R.V.I.S. online. All systems nominal, Sir.");
}

JarvisBackend::~JarvisBackend() = default;

void JarvisBackend::connectModuleSignals()
{
    // Audio → Backend
    connect(m_audio, &JarvisAudio::listeningChanged, this, &JarvisBackend::listeningChanged);
    connect(m_audio, &JarvisAudio::wakeWordActiveChanged, this, &JarvisBackend::wakeWordActiveChanged);
    connect(m_audio, &JarvisAudio::audioLevelChanged, this, &JarvisBackend::audioLevelChanged);
    connect(m_audio, &JarvisAudio::voiceCommandModeChanged, this, &JarvisBackend::voiceCommandModeChanged);
    connect(m_audio, &JarvisAudio::lastTranscriptionChanged, this, &JarvisBackend::lastTranscriptionChanged);
    connect(m_audio, &JarvisAudio::wakeWordDetected, this, [this]() {
        emit wakeWordDetected();
        setStatus("Wake word detected! Listening, Sir...");
    });
    connect(m_audio, &JarvisAudio::voiceCommandTranscribed, this, &JarvisBackend::onVoiceCommandTranscribed);

    // TTS → Backend
    connect(m_tts, &JarvisTts::speakingChanged, this, &JarvisBackend::speakingChanged);

    // System → Backend
    connect(m_system, &JarvisSystem::systemStatsChanged, this, &JarvisBackend::systemStatsChanged);
    connect(m_system, &JarvisSystem::currentTimeChanged, this, &JarvisBackend::currentTimeChanged);

    // Settings → Backend
    connect(m_settings, &JarvisSettings::llmServerUrlChanged, this, [this]() {
        emit llmServerUrlChanged();
        checkConnection();
    });
    connect(m_settings, &JarvisSettings::currentModelNameChanged, this, [this]() {
        emit currentModelNameChanged();
        setStatus(QStringLiteral("LLM model set to: ") + m_settings->currentModelName());
    });
    connect(m_settings, &JarvisSettings::currentVoiceNameChanged, this, [this]() {
        emit currentVoiceNameChanged();
        setStatus(QStringLiteral("Voice changed to: ") + m_settings->currentVoiceName());
    });
    connect(m_settings, &JarvisSettings::downloadProgressChanged, this, &JarvisBackend::downloadProgressChanged);
    connect(m_settings, &JarvisSettings::downloadingChanged, this, &JarvisBackend::downloadingChanged);
    connect(m_settings, &JarvisSettings::downloadStatusChanged, this, &JarvisBackend::downloadStatusChanged);
    connect(m_settings, &JarvisSettings::maxHistoryPairsChanged, this, &JarvisBackend::maxHistoryPairsChanged);
    connect(m_settings, &JarvisSettings::wakeBufferSecondsChanged, this, [this]() {
        m_audio->updateWakeBufferInterval(m_settings->wakeBufferSeconds());
        emit wakeBufferSecondsChanged();
    });
    connect(m_settings, &JarvisSettings::voiceCmdMaxSecondsChanged, this, [this]() {
        m_audio->updateVoiceCmdTimeout(m_settings->voiceCmdMaxSeconds());
        emit voiceCmdMaxSecondsChanged();
    });
    connect(m_settings, &JarvisSettings::autoStartWakeWordChanged, this, &JarvisBackend::autoStartWakeWordChanged);
    connect(m_settings, &JarvisSettings::personalityPromptChanged, this, &JarvisBackend::personalityPromptChanged);
    connect(m_settings, &JarvisSettings::ttsRateChanged, this, [this]() { m_tts->onTtsRateChanged(); });
    connect(m_settings, &JarvisSettings::ttsPitchChanged, this, [this]() { m_tts->onTtsPitchChanged(); });
    connect(m_settings, &JarvisSettings::ttsVolumeChanged, this, [this]() { m_tts->onTtsVolumeChanged(); });
    connect(m_settings, &JarvisSettings::ttsMutedChanged, this, &JarvisBackend::ttsMutedChanged);
    connect(m_settings, &JarvisSettings::voiceActivated, m_tts, &JarvisTts::onVoiceActivated);

    // Commands → Backend
    connect(m_commands, &JarvisCommands::commandMappingsChanged, this, &JarvisBackend::commandMappingsChanged);
    connect(m_commands, &JarvisCommands::commandExecuted, this, [this](const QString &phrase, const QString &action) {
        Q_UNUSED(action)
        setStatus(QStringLiteral("Executing: ") + phrase);
        addToChatHistory("jarvis", QStringLiteral("Executing command: %1").arg(phrase));
    });
    connect(m_commands, &JarvisCommands::commandOutput, this, [this](const QString &output) {
        addToChatHistory("jarvis", output);
    });

    // LLM Manager → Backend
    connect(m_llmManager, &JarvisLlmManager::serverRunningChanged, this, [this]() {
        emit llmServerRunningChanged();
        if (m_llmManager->isServerRunning()) {
            // Update settings URL to point to bundled server
            m_settings->setLlmServerUrl(m_llmManager->serverUrl());
            setStatus(QStringLiteral("Bundled LLM server started on port %1.").arg(m_llmManager->serverPort()));
            checkConnection();
        }
    });
    connect(m_llmManager, &JarvisLlmManager::serverError, this, [this](const QString &error) {
        setStatus(QStringLiteral("LLM server error: ") + error);
    });
    connect(m_llmManager, &JarvisLlmManager::serverStopped, this, [this]() {
        setStatus(QStringLiteral("LLM server stopped."));
    });

    // Whisper model activation → reload whisper
    connect(m_settings, &JarvisSettings::whisperModelActivated, this, [this](const QString &modelPath) {
        Q_UNUSED(modelPath)
        setStatus(QStringLiteral("Whisper model changed. Restart required for wake word."));
    });
    connect(m_settings, &JarvisSettings::currentWhisperModelChanged, this, &JarvisBackend::currentWhisperModelChanged);
    connect(m_settings, &JarvisSettings::availableWhisperModelsChanged, this, &JarvisBackend::availableWhisperModelsChanged);
    connect(m_settings, &JarvisSettings::piperInstalledChanged, this, &JarvisBackend::piperInstalledChanged);
}

// ─────────────────────────────────────────────
// Delegated getters
// ─────────────────────────────────────────────

bool JarvisBackend::isListening() const { return m_audio->isListening(); }
bool JarvisBackend::isWakeWordActive() const { return m_audio->isWakeWordActive(); }
double JarvisBackend::audioLevel() const { return m_audio->audioLevel(); }
bool JarvisBackend::isSpeaking() const { return m_tts->isSpeaking(); }
bool JarvisBackend::isTtsMuted() const { return m_tts->isMuted(); }
bool JarvisBackend::isVoiceCommandMode() const { return m_audio->isVoiceCommandMode(); }
QString JarvisBackend::lastTranscription() const { return m_audio->lastTranscription(); }

double JarvisBackend::cpuUsage() const { return m_system->cpuUsage(); }
double JarvisBackend::memoryUsage() const { return m_system->memoryUsage(); }
double JarvisBackend::memoryTotalGb() const { return m_system->memoryTotalGb(); }
double JarvisBackend::memoryUsedGb() const { return m_system->memoryUsedGb(); }
int JarvisBackend::cpuTemp() const { return m_system->cpuTemp(); }
QString JarvisBackend::uptime() const { return m_system->uptime(); }
QString JarvisBackend::hostname() const { return m_system->hostname(); }
QString JarvisBackend::kernelVersion() const { return m_system->kernelVersion(); }
QString JarvisBackend::currentTime() const { return m_system->currentTime(); }
QString JarvisBackend::currentDate() const { return m_system->currentDate(); }
QString JarvisBackend::greeting() const { return m_system->greeting(); }

QString JarvisBackend::llmServerUrl() const { return m_settings->llmServerUrl(); }
QString JarvisBackend::currentModelName() const { return m_settings->currentModelName(); }
QString JarvisBackend::currentVoiceName() const { return m_settings->currentVoiceName(); }
QVariantList JarvisBackend::availableLlmModels() const { return m_settings->availableLlmModels(); }
QVariantList JarvisBackend::availableTtsVoices() const { return m_settings->availableTtsVoices(); }
double JarvisBackend::downloadProgress() const { return m_settings->downloadProgress(); }
bool JarvisBackend::isDownloading() const { return m_settings->isDownloading(); }
QString JarvisBackend::downloadStatus() const { return m_settings->downloadStatus(); }
int JarvisBackend::maxHistoryPairs() const { return m_settings->maxHistoryPairs(); }
int JarvisBackend::wakeBufferSeconds() const { return m_settings->wakeBufferSeconds(); }
int JarvisBackend::voiceCmdMaxSeconds() const { return m_settings->voiceCmdMaxSeconds(); }
bool JarvisBackend::autoStartWakeWord() const { return m_settings->autoStartWakeWord(); }
QString JarvisBackend::personalityPrompt() const { return m_settings->personalityPrompt(); }

QVariantList JarvisBackend::commandMappings() const { return m_commands->commandMappings(); }

QVariantList JarvisBackend::availableWhisperModels() const { return m_settings->availableWhisperModels(); }
QString JarvisBackend::currentWhisperModel() const { return m_settings->currentWhisperModel(); }
bool JarvisBackend::piperInstalled() const { return m_settings->piperInstalled(); }
bool JarvisBackend::llmServerBundled() const { return m_settings->llmServerBundled(); }
bool JarvisBackend::isLlmServerRunning() const { return m_llmManager->isServerRunning(); }

// ─────────────────────────────────────────────
// Invokable delegates
// ─────────────────────────────────────────────

void JarvisBackend::speak(const QString &text) { m_tts->speak(text); }
void JarvisBackend::stopSpeaking() { m_tts->stop(); }
void JarvisBackend::toggleTtsMute() { m_tts->toggleMute(); }
void JarvisBackend::toggleWakeWord() { m_audio->toggleWakeWord(); }
void JarvisBackend::startVoiceCommand() { m_audio->startVoiceCommand(); }
void JarvisBackend::stopVoiceCommand() { m_audio->stopVoiceCommand(); }

void JarvisBackend::setTtsRate(double rate) { m_settings->setTtsRate(rate); }
void JarvisBackend::setTtsPitch(double pitch) { m_settings->setTtsPitch(pitch); }
void JarvisBackend::setTtsVolume(double volume) { m_settings->setTtsVolume(volume); }
void JarvisBackend::setLlmServerUrl(const QString &url) { m_settings->setLlmServerUrl(url); }
void JarvisBackend::downloadLlmModel(const QString &modelId) { m_settings->downloadLlmModel(modelId); }
void JarvisBackend::downloadTtsVoice(const QString &voiceId) { m_settings->downloadTtsVoice(voiceId); }
void JarvisBackend::setActiveLlmModel(const QString &modelId) { m_settings->setActiveLlmModel(modelId); }
void JarvisBackend::setActiveTtsVoice(const QString &voiceId) { m_settings->setActiveTtsVoice(voiceId); }
void JarvisBackend::setMaxHistoryPairs(int pairs) { m_settings->setMaxHistoryPairs(pairs); }
void JarvisBackend::setWakeBufferSeconds(int seconds) { m_settings->setWakeBufferSeconds(seconds); }
void JarvisBackend::setVoiceCmdMaxSeconds(int seconds) { m_settings->setVoiceCmdMaxSeconds(seconds); }
void JarvisBackend::setAutoStartWakeWord(bool enabled) { m_settings->setAutoStartWakeWord(enabled); }
void JarvisBackend::setPersonalityPrompt(const QString &prompt) { m_settings->setPersonalityPrompt(prompt); }
void JarvisBackend::cancelDownload() { m_settings->cancelDownload(); }

void JarvisBackend::downloadWhisperModel(const QString &modelId) { m_settings->downloadWhisperModel(modelId); }
void JarvisBackend::setActiveWhisperModel(const QString &modelId) { m_settings->setActiveWhisperModel(modelId); }
void JarvisBackend::downloadPiperBinary() { m_settings->downloadPiperBinary(); }

void JarvisBackend::startLlmServer() { m_llmManager->startServer(); }
void JarvisBackend::stopLlmServer() { m_llmManager->stopServer(); }
void JarvisBackend::restartLlmServer() { m_llmManager->restartServer(); }

void JarvisBackend::testVoice(const QString &voiceId)
{
    const QString voicesDir = m_settings->jarvisDataDir() + QStringLiteral("/piper-voices");
    const QString onnxPath = voicesDir + QStringLiteral("/") + voiceId + QStringLiteral(".onnx");
    if (!QFile::exists(onnxPath)) {
        qWarning() << "[JARVIS] testVoice: onnx not found:" << onnxPath;
        return;
    }

    QString piperBin = m_settings->piperBinaryPath();
    if (piperBin.isEmpty()) {
        for (const auto &path : {"/usr/lib/piper-tts/bin/piper", "/usr/bin/piper", "/usr/local/bin/piper"}) {
            if (QFile::exists(QString::fromLatin1(path))) { piperBin = QString::fromLatin1(path); break; }
        }
    }
    if (piperBin.isEmpty()) {
        qWarning() << "[JARVIS] testVoice: piper binary not found";
        return;
    }

    // Stop any current speech first
    m_tts->stop();

    const QString wavPath = QDir::tempPath() + QStringLiteral("/jarvis_voice_test.wav");

    // Use printf to avoid shell quoting issues with echo
    QString cmd = QStringLiteral(
        "printf '%s' 'At your service, Sir. All systems are nominal.' | "
        "'%1' -m '%2' -f '%3' --sentence-silence 0.3 2>/dev/null && "
        "pw-play '%3' 2>/dev/null; rm -f '%3'"
    ).arg(piperBin, onnxPath, wavPath);

    qDebug() << "[JARVIS] testVoice cmd:" << cmd;

    auto *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [proc](int exitCode, QProcess::ExitStatus) {
        if (exitCode != 0) {
            qWarning() << "[JARVIS] testVoice failed, exit:" << exitCode
                       << "stderr:" << QString::fromUtf8(proc->readAllStandardError());
        }
        proc->deleteLater();
    });
    proc->setProcessChannelMode(QProcess::MergedChannels);
    proc->start(QStringLiteral("/bin/sh"), {QStringLiteral("-c"), cmd});
}

void JarvisBackend::fetchMoreModels()
{
    m_settings->fetchMoreModels();
    emit availableLlmModelsChanged();
}

void JarvisBackend::fetchMoreVoices()
{
    m_settings->fetchMoreVoices();
    emit availableTtsVoicesChanged();
}

void JarvisBackend::addCommand(const QString &phrase, const QString &action, const QString &type) { m_commands->addCommand(phrase, action, type); }
void JarvisBackend::removeCommand(int index) { m_commands->removeCommand(index); }
void JarvisBackend::updateCommand(int index, const QString &phrase, const QString &action, const QString &type) { m_commands->updateCommand(index, phrase, action, type); }
void JarvisBackend::resetCommandsToDefaults() { m_commands->resetToDefaults(); }

void JarvisBackend::openUrl(const QString &url)
{
    QProcess::startDetached(QStringLiteral("xdg-open"), {url});
}

// ─────────────────────────────────────────────
// Voice Command Processing
// ─────────────────────────────────────────────

void JarvisBackend::onVoiceCommandTranscribed(const QString &text)
{
    if (text.isEmpty()) {
        setStatus("I couldn't make out what you said, Sir.");
        return;
    }

    qDebug() << "[JARVIS] Voice command transcribed:" << text;

    // Remove wake word from transcription if present
    QString command = text;
    command.remove(QRegularExpression(QStringLiteral("^\\s*jarvis[,\\s]*"),
                                      QRegularExpression::CaseInsensitiveOption));
    command = command.trimmed();

    if (command.isEmpty()) {
        setStatus("Yes, Sir? I'm listening.");
        speak(QStringLiteral("Yes, Sir?"));
        return;
    }

    // Try system commands first
    if (m_commands->tryExecuteVoiceCommand(command)) {
        speak(QStringLiteral("Right away, Sir."));
        return;
    }

    // Otherwise send to LLM
    sendMessage(command);
}

// ─────────────────────────────────────────────
// LLM Communication
// ─────────────────────────────────────────────

void JarvisBackend::sendMessage(const QString &message)
{
    if (message.trimmed().isEmpty()) return;

    addToChatHistory("user", message);
    sendToLlm(message);
}

void JarvisBackend::sendToLlm(const QString &userMessage)
{
    m_processing = true;
    emit processingChanged();
    setStatus("Processing your request, Sir...");

    m_conversationHistory.push_back({QStringLiteral("user"), userMessage});

    const int maxPairs = m_settings->maxHistoryPairs();
    while (m_conversationHistory.size() > static_cast<size_t>(maxPairs * 2)) {
        m_conversationHistory.erase(m_conversationHistory.begin());
    }

    QJsonArray messages = buildConversationContext();

    QJsonObject requestBody;
    requestBody[QStringLiteral("messages")] = messages;
    requestBody[QStringLiteral("temperature")] = 0.7;
    requestBody[QStringLiteral("max_tokens")] = 2048;
    requestBody[QStringLiteral("stream")] = true;

    const QUrl url(m_settings->llmServerUrl() + QStringLiteral("/v1/chat/completions"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setTransferTimeout(120000);

    // Reset streaming state
    m_streamBuffer.clear();
    m_fullStreamedResponse.clear();
    m_spokenSoFar.clear();
    m_streamingResponse.clear();
    emit streamingResponseChanged();

    m_streamReply = m_networkManager->post(request, QJsonDocument(requestBody).toJson());
    connect(m_streamReply, &QNetworkReply::readyRead, this, &JarvisBackend::onLlmStreamReadyRead);
    connect(m_streamReply, &QNetworkReply::finished, this, &JarvisBackend::onLlmStreamFinished);
}

QJsonArray JarvisBackend::buildConversationContext() const
{
    QJsonArray messages;

    QString systemPrompt = m_settings->personalityPrompt().isEmpty()
        ? QString::fromUtf8(JARVIS_SYSTEM_PROMPT) : m_settings->personalityPrompt();
    systemPrompt += QStringLiteral("\n\nCurrent system status:\n");
    systemPrompt += QStringLiteral("- CPU Usage: %1%\n").arg(m_system->cpuUsage(), 0, 'f', 1);
    systemPrompt += QStringLiteral("- Memory: %1 / %2 GB (%3%)\n")
                        .arg(m_system->memoryUsedGb(), 0, 'f', 1)
                        .arg(m_system->memoryTotalGb(), 0, 'f', 1)
                        .arg(m_system->memoryUsage(), 0, 'f', 1);
    systemPrompt += QStringLiteral("- CPU Temperature: %1°C\n").arg(m_system->cpuTemp());
    systemPrompt += QStringLiteral("- Uptime: %1\n").arg(m_system->uptime());
    systemPrompt += QStringLiteral("- Hostname: %1\n").arg(m_system->hostname());
    systemPrompt += QStringLiteral("- Current time: %1 %2\n").arg(m_system->currentTime(), m_system->currentDate());

    if (!m_reminders.empty()) {
        systemPrompt += QStringLiteral("- Active reminders: %1\n").arg(m_reminders.size());
    }

    QJsonObject systemMsg;
    systemMsg[QStringLiteral("role")] = QStringLiteral("system");
    systemMsg[QStringLiteral("content")] = systemPrompt;
    messages.append(systemMsg);

    for (const auto &[role, content] : m_conversationHistory) {
        QJsonObject msg;
        msg[QStringLiteral("role")] = role;
        msg[QStringLiteral("content")] = content;
        messages.append(msg);
    }

    return messages;
}

void JarvisBackend::onLlmStreamReadyRead()
{
    if (!m_streamReply) return;

    const QByteArray data = m_streamReply->readAll();
    m_streamBuffer += QString::fromUtf8(data);

    // Process SSE lines: each chunk is "data: {...}\n\n"
    while (true) {
        const int nlPos = m_streamBuffer.indexOf(QLatin1Char('\n'));
        if (nlPos < 0) break;

        const QString line = m_streamBuffer.left(nlPos).trimmed();
        m_streamBuffer = m_streamBuffer.mid(nlPos + 1);

        if (line.isEmpty() || line == QStringLiteral("data: [DONE]")) {
            continue;
        }

        if (!line.startsWith(QStringLiteral("data: "))) {
            continue;
        }

        const QString jsonStr = line.mid(6); // skip "data: "
        const auto doc = QJsonDocument::fromJson(jsonStr.toUtf8());
        if (doc.isNull()) continue;

        const auto obj = doc.object();
        const auto choices = obj[QStringLiteral("choices")].toArray();
        if (choices.isEmpty()) continue;

        const auto delta = choices[0].toObject()[QStringLiteral("delta")].toObject();
        const QString content = delta[QStringLiteral("content")].toString();

        if (!content.isEmpty()) {
            m_fullStreamedResponse += content;

            // Update streaming response for QML (strip action blocks for display)
            m_streamingResponse = stripActionsFromResponse(m_fullStreamedResponse);
            emit streamingResponseChanged();

            // Try to speak any complete sentences that haven't been spoken yet
            trySpeakCompleteSentences();
        }
    }
}

void JarvisBackend::trySpeakCompleteSentences()
{
    // Get the displayable text so far (without action blocks)
    const QString displayText = stripActionsFromResponse(m_fullStreamedResponse);

    // Find complete sentences that we haven't spoken yet
    static const QRegularExpression sentenceEndRe(QStringLiteral("[.!?;:]\\s"));
    const int searchStart = m_spokenSoFar.length();
    if (searchStart >= displayText.length()) return;

    const QString unspoken = displayText.mid(searchStart);
    const auto match = sentenceEndRe.match(unspoken);

    if (match.hasMatch()) {
        // We have at least one complete sentence
        const int endPos = match.capturedStart() + 1; // include the punctuation
        const QString sentence = unspoken.left(endPos).trimmed();

        if (!sentence.isEmpty()) {
            m_spokenSoFar = displayText.left(searchStart + endPos);
            m_tts->speakSentence(sentence);
        }
    }
}

void JarvisBackend::finalizeStreamingResponse()
{
    const QString responseText = m_fullStreamedResponse.trimmed();

    if (responseText.isEmpty()) {
        m_lastResponse = QStringLiteral("I apologize, Sir. I wasn't able to formulate a response.");
        emit lastResponseChanged();
        addToChatHistory("jarvis", m_lastResponse);
        setStatus("Ready.");
        return;
    }

    m_conversationHistory.push_back({QStringLiteral("assistant"), responseText});

    const QString spokenText = stripActionsFromResponse(responseText);

    m_lastResponse = spokenText;
    m_streamingResponse.clear();
    emit lastResponseChanged();
    emit streamingResponseChanged();

    addToChatHistory("jarvis", spokenText);
    setStatus("Ready.");
    emit responseReceived(spokenText);

    // Speak any remaining unspoken text
    const QString remaining = spokenText.mid(m_spokenSoFar.length()).trimmed();
    if (!remaining.isEmpty()) {
        m_tts->speakSentence(remaining);
    }

    // Parse and execute any actions embedded in the LLM response
    parseAndExecuteActions(responseText);
}

void JarvisBackend::onLlmStreamFinished()
{
    if (!m_streamReply) return;

    const auto error = m_streamReply->error();
    const QString errorString = m_streamReply->errorString();

    // Process any remaining data before cleanup
    const QByteArray remaining = m_streamReply->readAll();
    if (!remaining.isEmpty()) {
        m_streamBuffer += QString::fromUtf8(remaining);
    }

    m_streamReply->deleteLater();
    m_streamReply = nullptr;

    m_processing = false;
    emit processingChanged();

    if (error != QNetworkReply::NoError && m_fullStreamedResponse.isEmpty()) {
        const auto errorMsg = QStringLiteral("I'm experiencing a connection issue, Sir: %1")
                                  .arg(errorString);
        setStatus(errorMsg);
        emit errorOccurred(errorMsg);
        if (!m_conversationHistory.empty()) {
            m_conversationHistory.pop_back();
        }
        return;
    }

    // Parse any remaining SSE lines in buffer
    while (true) {
        const int nlPos = m_streamBuffer.indexOf(QLatin1Char('\n'));
        if (nlPos < 0) break;

        const QString line = m_streamBuffer.left(nlPos).trimmed();
        m_streamBuffer = m_streamBuffer.mid(nlPos + 1);

        if (line.isEmpty() || line == QStringLiteral("data: [DONE]")) continue;
        if (!line.startsWith(QStringLiteral("data: "))) continue;

        const auto doc = QJsonDocument::fromJson(line.mid(6).toUtf8());
        if (doc.isNull()) continue;

        const auto choices = doc.object()[QStringLiteral("choices")].toArray();
        if (choices.isEmpty()) continue;

        const QString content = choices[0].toObject()[QStringLiteral("delta")].toObject()
                                    [QStringLiteral("content")].toString();
        if (!content.isEmpty()) {
            m_fullStreamedResponse += content;
        }
    }

    // Finalize
    finalizeStreamingResponse();
}

void JarvisBackend::checkConnection()
{
    const QUrl url(m_settings->llmServerUrl() + QStringLiteral("/health"));
    QNetworkRequest request(url);
    request.setTransferTimeout(5000);

    auto *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onHealthCheckFinished(reply);
    });
}

void JarvisBackend::onHealthCheckFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    const bool wasConnected = m_connected;
    m_connected = (reply->error() == QNetworkReply::NoError);

    if (m_connected != wasConnected) {
        emit connectedChanged();
        if (m_connected) {
            setStatus("LLM server connected. All systems operational, Sir.");
        } else {
            setStatus("LLM server offline. Attempting to reconnect...");
        }
    }
}

// ─────────────────────────────────────────────
// Reminders
// ─────────────────────────────────────────────

void JarvisBackend::addReminder(const QString &text, int secondsFromNow)
{
    Reminder r;
    r.text = text;
    r.triggerTime = QDateTime::currentDateTime().addSecs(secondsFromNow);
    m_reminders.push_back(r);

    QVariantMap map;
    map[QStringLiteral("text")] = text;
    map[QStringLiteral("time")] = r.triggerTime.toString(QStringLiteral("HH:mm:ss"));
    m_activeReminders.append(map);
    emit remindersChanged();

    setStatus(QStringLiteral("Reminder set, Sir. I'll notify you at %1.")
                  .arg(r.triggerTime.toString(QStringLiteral("HH:mm"))));
}

void JarvisBackend::removeReminder(int index)
{
    if (index < 0 || index >= static_cast<int>(m_reminders.size())) return;

    m_reminders.erase(m_reminders.begin() + index);
    m_activeReminders.removeAt(index);
    emit remindersChanged();
}

void JarvisBackend::checkReminders()
{
    const auto now = QDateTime::currentDateTime();
    bool changed = false;

    auto it = m_reminders.begin();
    int idx = 0;
    while (it != m_reminders.end()) {
        if (now >= it->triggerTime) {
            const QString text = it->text;
            emit reminderTriggered(text);
            speak(QStringLiteral("Excuse me, Sir. Reminder: %1").arg(text));
            addToChatHistory("jarvis", QStringLiteral("⏰ Reminder: %1").arg(text));

            it = m_reminders.erase(it);
            if (idx < m_activeReminders.size()) {
                m_activeReminders.removeAt(idx);
            }
            changed = true;
        } else {
            ++it;
            ++idx;
        }
    }

    if (changed) {
        emit remindersChanged();
    }
}

// ─────────────────────────────────────────────
// Misc
// ─────────────────────────────────────────────

void JarvisBackend::clearHistory()
{
    m_chatHistory.clear();
    m_conversationHistory.clear();
    m_lastResponse.clear();
    emit chatHistoryChanged();
    emit lastResponseChanged();
    setStatus("Memory cleared, Sir. Fresh start.");
}

void JarvisBackend::setStatus(const QString &status)
{
    if (m_statusText != status) {
        m_statusText = status;
        emit statusTextChanged();
    }
}

void JarvisBackend::addToChatHistory(const QString &role, const QString &message)
{
    m_chatHistory.append(QStringLiteral("%1|%2").arg(role, message));

    const int maxPairs = m_settings->maxHistoryPairs();
    while (m_chatHistory.size() > maxPairs * 2) {
        m_chatHistory.removeFirst();
    }

    emit chatHistoryChanged();
}

// ─────────────────────────────────────────────
// Action Parsing & Execution
// ─────────────────────────────────────────────

QString JarvisBackend::expandPath(const QString &path) const
{
    QString expanded = path.trimmed();
    const QString home = QDir::homePath();
    if (expanded.startsWith(QStringLiteral("~/"))) {
        expanded = home + expanded.mid(1);
    } else if (expanded == QStringLiteral("~")) {
        expanded = home;
    }
    expanded.replace(QStringLiteral("$HOME"), home);
    return expanded;
}

QString JarvisBackend::stripActionsFromResponse(const QString &responseText) const
{
    QString cleaned = responseText;

    // Remove [ACTION:...] lines and CONTENT_START/CONTENT_END blocks
    static const QRegularExpression actionLineRe(
        QStringLiteral("\\[ACTION:[^\\]]+\\].*"),
        QRegularExpression::MultilineOption);
    static const QRegularExpression contentBlockRe(
        QStringLiteral("CONTENT_START\\n[\\s\\S]*?CONTENT_END"));

    cleaned.remove(contentBlockRe);
    cleaned.remove(actionLineRe);

    // Clean up extra whitespace
    cleaned = cleaned.trimmed();
    static const QRegularExpression multiNewline(QStringLiteral("\\n{3,}"));
    cleaned.replace(multiNewline, QStringLiteral("\n\n"));

    return cleaned;
}

void JarvisBackend::parseAndExecuteActions(const QString &responseText)
{
    // Parse [ACTION:type] arg lines
    static const QRegularExpression actionRe(
        QStringLiteral("\\[ACTION:(\\w+)\\]\\s*(.*)"),
        QRegularExpression::MultilineOption);

    auto it = actionRe.globalMatch(responseText);
    while (it.hasNext()) {
        const auto match = it.next();
        const QString actionType = match.captured(1).toLower();
        const QString arg = match.captured(2).trimmed();

        qDebug() << "[JARVIS] Action:" << actionType << "arg:" << arg;

        if (actionType == QStringLiteral("run_command")) {
            executeRunCommand(arg);
        } else if (actionType == QStringLiteral("open_terminal")) {
            executeOpenTerminal(arg);
        } else if (actionType == QStringLiteral("write_file")) {
            // Extract content between CONTENT_START and CONTENT_END after this action
            const int actionPos = responseText.indexOf(match.captured(0));
            const int csPos = responseText.indexOf(QStringLiteral("CONTENT_START"), actionPos);
            const int cePos = responseText.indexOf(QStringLiteral("CONTENT_END"), csPos);
            if (csPos >= 0 && cePos > csPos) {
                const int contentStart = csPos + QStringLiteral("CONTENT_START").length();
                QString content = responseText.mid(contentStart, cePos - contentStart);
                // Remove leading/trailing newline only (preserve internal formatting)
                if (content.startsWith(QLatin1Char('\n'))) content = content.mid(1);
                if (content.endsWith(QLatin1Char('\n'))) content.chop(1);
                executeWriteFile(arg, content);
            } else {
                qWarning() << "[JARVIS] write_file action missing CONTENT_START/CONTENT_END block";
            }
        } else if (actionType == QStringLiteral("open_app")) {
            executeOpenApp(arg);
        } else if (actionType == QStringLiteral("open_url")) {
            openUrl(arg);
        } else if (actionType == QStringLiteral("type_text")) {
            executeTypeText(arg);
        } else {
            qWarning() << "[JARVIS] Unknown action type:" << actionType;
        }
    }
}

void JarvisBackend::executeRunCommand(const QString &command)
{
    if (command.isEmpty()) return;
    qDebug() << "[JARVIS] Executing command:" << command;

    auto *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc, command](int exitCode, QProcess::ExitStatus) {
        const QString out = QString::fromUtf8(proc->readAllStandardOutput()).trimmed();
        const QString err = QString::fromUtf8(proc->readAllStandardError()).trimmed();
        if (!out.isEmpty()) {
            addToChatHistory("system", QStringLiteral("Command output: %1").arg(out));
        }
        if (exitCode != 0 && !err.isEmpty()) {
            addToChatHistory("system", QStringLiteral("Command error: %1").arg(err));
        }
        proc->deleteLater();
    });

    // Expand ~ in the command
    const QString expanded = command.contains(QStringLiteral("~"))
        ? QString(command).replace(QStringLiteral("~"), QDir::homePath())
        : command;

    proc->start(QStringLiteral("/bin/sh"), {QStringLiteral("-c"), expanded});
}

void JarvisBackend::executeOpenTerminal(const QString &command)
{
    if (command.isEmpty()) return;
    qDebug() << "[JARVIS] Opening terminal with command:" << command;

    const QString expanded = command.contains(QStringLiteral("~"))
        ? QString(command).replace(QStringLiteral("~"), QDir::homePath())
        : command;

    // Try common terminal emulators with execute flag
    const QStringList terminals = {
        QStringLiteral("konsole"),
        QStringLiteral("gnome-terminal"),
        QStringLiteral("xfce4-terminal"),
        QStringLiteral("xterm"),
    };

    for (const auto &term : terminals) {
        auto *which = new QProcess(this);
        which->start(QStringLiteral("which"), {term});
        which->waitForFinished(1000);
        if (which->exitCode() == 0) {
            delete which;
            if (term == QStringLiteral("konsole")) {
                QProcess::startDetached(term, {QStringLiteral("-e"), QStringLiteral("/bin/sh"), QStringLiteral("-c"), expanded + QStringLiteral("; exec $SHELL")});
            } else if (term == QStringLiteral("gnome-terminal")) {
                QProcess::startDetached(term, {QStringLiteral("--"), QStringLiteral("/bin/sh"), QStringLiteral("-c"), expanded + QStringLiteral("; exec $SHELL")});
            } else {
                QProcess::startDetached(term, {QStringLiteral("-e"), QStringLiteral("/bin/sh"), QStringLiteral("-c"), expanded + QStringLiteral("; exec $SHELL")});
            }
            addToChatHistory("system", QStringLiteral("Opened terminal: %1").arg(command));
            return;
        }
        delete which;
    }
    qWarning() << "[JARVIS] No terminal emulator found";
}

void JarvisBackend::executeWriteFile(const QString &path, const QString &content)
{
    const QString expanded = expandPath(path);
    qDebug() << "[JARVIS] Writing file:" << expanded << "content length:" << content.length();

    // Create parent directories
    const QFileInfo fi(expanded);
    QDir().mkpath(fi.absolutePath());

    QFile file(expanded);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(content.toUtf8());
        file.close();
        addToChatHistory("system", QStringLiteral("File written: %1 (%2 bytes)").arg(expanded).arg(content.size()));
        qDebug() << "[JARVIS] File written successfully:" << expanded;
    } else {
        const QString err = QStringLiteral("Failed to write file: %1 — %2").arg(expanded, file.errorString());
        addToChatHistory("system", err);
        qWarning() << "[JARVIS]" << err;
    }
}

void JarvisBackend::executeOpenApp(const QString &app)
{
    if (app.isEmpty()) return;
    qDebug() << "[JARVIS] Opening app:" << app;

    // If it contains a path (like ~/Desktop/file.md), open with xdg-open
    if (app.contains(QStringLiteral("/")) || app.contains(QStringLiteral("~"))) {
        const QString expanded = expandPath(app);
        QProcess::startDetached(QStringLiteral("xdg-open"), {expanded});
        addToChatHistory("system", QStringLiteral("Opened: %1").arg(expanded));
        return;
    }

    // Try to find and launch the app, possibly with arguments
    const auto parts = app.split(QLatin1Char(' '));
    const QString bin = parts.first();
    QStringList args = parts.mid(1);

    // Expand paths in args
    for (auto &a : args) {
        a = expandPath(a);
    }

    auto *which = new QProcess(this);
    which->start(QStringLiteral("which"), {bin});
    which->waitForFinished(1000);
    if (which->exitCode() == 0) {
        delete which;
        QProcess::startDetached(bin, args);
        addToChatHistory("system", QStringLiteral("Launched: %1").arg(app));
    } else {
        delete which;
        // Try with xdg-open as fallback
        QProcess::startDetached(QStringLiteral("xdg-open"), {app});
        addToChatHistory("system", QStringLiteral("Attempted to open: %1").arg(app));
    }
}

void JarvisBackend::executeTypeText(const QString &text)
{
    if (text.isEmpty()) return;
    qDebug() << "[JARVIS] Typing text:" << text.left(50) << "...";

    // Use xdotool to type text into the focused window
    // Small delay to allow window focus to settle
    auto *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            proc, &QProcess::deleteLater);

    // xdotool type with a small delay between keystrokes for reliability
    const QString escapedText = QString(text).replace(QStringLiteral("'"), QStringLiteral("'\\''"));
    const QString cmd = QStringLiteral("sleep 0.5 && xdotool type --clearmodifiers --delay 12 '%1'").arg(escapedText);
    proc->start(QStringLiteral("/bin/sh"), {QStringLiteral("-c"), cmd});
}
