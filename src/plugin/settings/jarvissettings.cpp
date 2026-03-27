#include "jarvissettings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QProcess>

JarvisSettings::JarvisSettings(QNetworkAccessManager *nam, QObject *parent)
    : QObject(parent)
    , m_networkManager(nam)
{
    loadSettings();
    populateModelList();
    populateVoiceList();
    populateWhisperModelList();
    detectPiperBinary();
}

// ─────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────

QString JarvisSettings::jarvisDataDir() const
{
    const QString dir = QDir::homePath() + QStringLiteral("/.local/share/jarvis");
    QDir().mkpath(dir);
    return dir;
}

// ─────────────────────────────────────────────
// Persistence
// ─────────────────────────────────────────────

void JarvisSettings::loadSettings()
{
    m_llmServerUrl = m_settings.value(QStringLiteral("llm/serverUrl"),
                                       QStringLiteral("http://127.0.0.1:8080")).toString();
    m_currentModelName = m_settings.value(QStringLiteral("llm/modelName"),
                                           QStringLiteral("Qwen2.5-Coder-1.5B-Instruct")).toString();
    m_currentVoiceName = m_settings.value(QStringLiteral("tts/voiceName"),
                                           QStringLiteral("en_GB-alan-medium")).toString();
    m_maxHistoryPairs = m_settings.value(QStringLiteral("chat/maxHistoryPairs"), 20).toInt();
    m_wakeBufferSeconds = m_settings.value(QStringLiteral("audio/wakeBufferSeconds"), 2).toInt();
    m_voiceCmdMaxSeconds = m_settings.value(QStringLiteral("audio/voiceCmdMaxSeconds"), 8).toInt();
    m_autoStartWakeWord = m_settings.value(QStringLiteral("audio/autoStartWakeWord"), true).toBool();
    m_ttsRate = m_settings.value(QStringLiteral("tts/rate"), 0.05).toDouble();
    m_ttsPitch = m_settings.value(QStringLiteral("tts/pitch"), -0.1).toDouble();
    m_ttsVolume = m_settings.value(QStringLiteral("tts/volume"), 0.85).toDouble();
    m_ttsMuted = m_settings.value(QStringLiteral("tts/muted"), false).toBool();
    m_personalityPrompt = m_settings.value(QStringLiteral("chat/personalityPrompt")).toString();

    // Resolve piper model path from saved voice name
    const QString voicesDir = jarvisDataDir() + QStringLiteral("/piper-voices");
    const QString savedPath = voicesDir + QStringLiteral("/") + m_currentVoiceName + QStringLiteral(".onnx");
    if (QFile::exists(savedPath)) {
        m_piperModelPath = savedPath;
    } else {
        // Fallback search
        const QStringList fallbackPaths = {
            QDir::homePath() + QStringLiteral("/.local/share/jarvis/piper-voices/en_GB-alan-medium.onnx"),
            QStringLiteral("/usr/share/jarvis/piper-voices/en_GB-alan-medium.onnx"),
        };
        for (const auto &path : fallbackPaths) {
            if (QFile::exists(path)) { m_piperModelPath = path; break; }
        }
    }

    // Resolve whisper model path
    m_currentWhisperModel = m_settings.value(QStringLiteral("audio/whisperModel"),
                                              QStringLiteral("ggml-tiny.en")).toString();
    const QString whisperDir = jarvisDataDir() + QStringLiteral("/whisper-models");
    const QString whisperSaved = whisperDir + QStringLiteral("/") + m_currentWhisperModel + QStringLiteral(".bin");
    if (QFile::exists(whisperSaved)) {
        m_whisperModelPath = whisperSaved;
    } else {
        // Fallback search in legacy paths
        const QStringList fallbackPaths = {
            QDir::homePath() + QStringLiteral("/.local/share/jarvis/ggml-tiny.bin"),
            QDir::homePath() + QStringLiteral("/.local/share/jarvis/ggml-tiny.en.bin"),
            QStringLiteral("/usr/share/jarvis/ggml-tiny.bin"),
            QStringLiteral("/usr/share/jarvis/ggml-tiny.en.bin"),
        };
        for (const auto &path : fallbackPaths) {
            if (QFile::exists(path)) { m_whisperModelPath = path; break; }
        }
    }
}

void JarvisSettings::saveSettings()
{
    m_settings.setValue(QStringLiteral("llm/serverUrl"), m_llmServerUrl);
    m_settings.setValue(QStringLiteral("llm/modelName"), m_currentModelName);
    m_settings.setValue(QStringLiteral("tts/voiceName"), m_currentVoiceName);
    m_settings.setValue(QStringLiteral("chat/maxHistoryPairs"), m_maxHistoryPairs);
    m_settings.setValue(QStringLiteral("audio/wakeBufferSeconds"), m_wakeBufferSeconds);
    m_settings.setValue(QStringLiteral("audio/voiceCmdMaxSeconds"), m_voiceCmdMaxSeconds);
    m_settings.setValue(QStringLiteral("audio/autoStartWakeWord"), m_autoStartWakeWord);
    m_settings.setValue(QStringLiteral("tts/rate"), m_ttsRate);
    m_settings.setValue(QStringLiteral("tts/pitch"), m_ttsPitch);
    m_settings.setValue(QStringLiteral("tts/volume"), m_ttsVolume);
    m_settings.setValue(QStringLiteral("tts/muted"), m_ttsMuted);
    m_settings.setValue(QStringLiteral("chat/personalityPrompt"), m_personalityPrompt);
    m_settings.setValue(QStringLiteral("audio/whisperModel"), m_currentWhisperModel);
    m_settings.sync();
}

// ─────────────────────────────────────────────
// Setters
// ─────────────────────────────────────────────

void JarvisSettings::setLlmServerUrl(const QString &url)
{
    if (m_llmServerUrl != url) {
        m_llmServerUrl = url;
        saveSettings();
        emit llmServerUrlChanged();
    }
}

void JarvisSettings::setCurrentModelName(const QString &name)
{
    if (m_currentModelName != name) {
        m_currentModelName = name;
        saveSettings();
        emit currentModelNameChanged();
    }
}

void JarvisSettings::setMaxHistoryPairs(int pairs)
{
    pairs = qBound(5, pairs, 100);
    if (m_maxHistoryPairs != pairs) {
        m_maxHistoryPairs = pairs;
        saveSettings();
        emit maxHistoryPairsChanged();
    }
}

void JarvisSettings::setWakeBufferSeconds(int seconds)
{
    seconds = qBound(1, seconds, 5);
    if (m_wakeBufferSeconds != seconds) {
        m_wakeBufferSeconds = seconds;
        saveSettings();
        emit wakeBufferSecondsChanged();
    }
}

void JarvisSettings::setVoiceCmdMaxSeconds(int seconds)
{
    seconds = qBound(3, seconds, 30);
    if (m_voiceCmdMaxSeconds != seconds) {
        m_voiceCmdMaxSeconds = seconds;
        saveSettings();
        emit voiceCmdMaxSecondsChanged();
    }
}

void JarvisSettings::setAutoStartWakeWord(bool enabled)
{
    if (m_autoStartWakeWord != enabled) {
        m_autoStartWakeWord = enabled;
        saveSettings();
        emit autoStartWakeWordChanged();
    }
}

void JarvisSettings::setPersonalityPrompt(const QString &prompt)
{
    if (m_personalityPrompt != prompt) {
        m_personalityPrompt = prompt;
        saveSettings();
        emit personalityPromptChanged();
    }
}

void JarvisSettings::setTtsRate(double rate)
{
    m_ttsRate = qBound(-1.0, rate, 1.0);
    saveSettings();
    emit ttsRateChanged();
}

void JarvisSettings::setTtsPitch(double pitch)
{
    m_ttsPitch = qBound(-1.0, pitch, 1.0);
    saveSettings();
    emit ttsPitchChanged();
}

void JarvisSettings::setTtsVolume(double volume)
{
    m_ttsVolume = qBound(0.0, volume, 1.0);
    saveSettings();
    emit ttsVolumeChanged();
}

void JarvisSettings::setTtsMuted(bool muted)
{
    if (m_ttsMuted != muted) {
        m_ttsMuted = muted;
        saveSettings();
        emit ttsMutedChanged();
    }
}

// ─────────────────────────────────────────────
// Model & Voice Lists
// ─────────────────────────────────────────────

void JarvisSettings::populateModelList()
{
    // All available models
    const QVariantList allModels = {
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("Qwen2.5-0.5B-Instruct-Q4_K_M")},
            {QStringLiteral("name"), QStringLiteral("Qwen 2.5 0.5B (Tiny)")},
            {QStringLiteral("size"), QStringLiteral("0.4 GB")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/bartowski/Qwen2.5-0.5B-Instruct-GGUF/resolve/main/Qwen2.5-0.5B-Instruct-Q4_K_M.gguf")},
            {QStringLiteral("desc"), QStringLiteral("Ultra-light, fast responses, basic quality")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("Qwen2.5-1.5B-Instruct-Q4_K_M")},
            {QStringLiteral("name"), QStringLiteral("Qwen 2.5 1.5B (Small)")},
            {QStringLiteral("size"), QStringLiteral("1.1 GB")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/bartowski/Qwen2.5-Coder-1.5B-Instruct-GGUF/resolve/main/Qwen2.5-Coder-1.5B-Instruct-Q4_K_M.gguf")},
            {QStringLiteral("desc"), QStringLiteral("Good balance of speed and quality")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("Qwen2.5-3B-Instruct-Q4_K_M")},
            {QStringLiteral("name"), QStringLiteral("Qwen 2.5 3B (Medium)")},
            {QStringLiteral("size"), QStringLiteral("2.0 GB")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/bartowski/Qwen2.5-3B-Instruct-GGUF/resolve/main/Qwen2.5-3B-Instruct-Q4_K_M.gguf")},
            {QStringLiteral("desc"), QStringLiteral("Better reasoning, moderate speed")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("Qwen2.5-7B-Instruct-Q4_K_M")},
            {QStringLiteral("name"), QStringLiteral("Qwen 2.5 7B (Large)")},
            {QStringLiteral("size"), QStringLiteral("4.7 GB")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/bartowski/Qwen2.5-7B-Instruct-GGUF/resolve/main/Qwen2.5-7B-Instruct-Q4_K_M.gguf")},
            {QStringLiteral("desc"), QStringLiteral("High quality, needs more RAM")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("Mistral-7B-Instruct-v0.3-Q4_K_M")},
            {QStringLiteral("name"), QStringLiteral("Mistral 7B v0.3")},
            {QStringLiteral("size"), QStringLiteral("4.4 GB")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/bartowski/Mistral-7B-Instruct-v0.3-GGUF/resolve/main/Mistral-7B-Instruct-v0.3-Q4_K_M.gguf")},
            {QStringLiteral("desc"), QStringLiteral("Excellent general assistant")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("Phi-3.5-mini-instruct-Q4_K_M")},
            {QStringLiteral("name"), QStringLiteral("Phi 3.5 Mini (3.8B)")},
            {QStringLiteral("size"), QStringLiteral("2.6 GB")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/bartowski/Phi-3.5-mini-instruct-GGUF/resolve/main/Phi-3.5-mini-instruct-Q4_K_M.gguf")},
            {QStringLiteral("desc"), QStringLiteral("Microsoft's compact powerhouse")}
        },
    };

    // Only show downloaded models
    m_availableLlmModels.clear();
    const QString modelsDir = jarvisDataDir() + QStringLiteral("/models");
    QDir().mkpath(modelsDir);
    
    for (auto v : allModels) {
        auto map = v.toMap();
        const QString filename = map[QStringLiteral("id")].toString() + QStringLiteral(".gguf");
        const QString filePath = modelsDir + QStringLiteral("/") + filename;
        
        if (QFile::exists(filePath)) {
            map[QStringLiteral("downloaded")] = true;
            map[QStringLiteral("active")] = (map[QStringLiteral("id")].toString() == m_currentModelName ||
                                          map[QStringLiteral("name")].toString().contains(m_currentModelName));
            m_availableLlmModels.append(map);
        }
    }
}

void JarvisSettings::populateVoiceList()
{
    // All available voices
    const QVariantList allVoices = {
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("en_GB-alan-medium")},
            {QStringLiteral("name"), QStringLiteral("Alan (British Male)")},
            {QStringLiteral("lang"), QStringLiteral("English (UK)")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_GB/alan/medium/en_GB-alan-medium.onnx")},
            {QStringLiteral("urlJson"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_GB/alan/medium/en_GB-alan-medium.onnx.json")},
            {QStringLiteral("desc"), QStringLiteral("Recommended — closest to J.A.R.V.I.S.")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("en_GB-cori-high")},
            {QStringLiteral("name"), QStringLiteral("Cori (British Female)")},
            {QStringLiteral("lang"), QStringLiteral("English (UK)")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_GB/cori/high/en_GB-cori-high.onnx")},
            {QStringLiteral("urlJson"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_GB/cori/high/en_GB-cori-high.onnx.json")},
            {QStringLiteral("desc"), QStringLiteral("British female — F.R.I.D.A.Y. style")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("en_US-joe-medium")},
            {QStringLiteral("name"), QStringLiteral("Joe (American Male)")},
            {QStringLiteral("lang"), QStringLiteral("English (US)")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/joe/medium/en_US-joe-medium.onnx")},
            {QStringLiteral("urlJson"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/joe/medium/en_US-joe-medium.onnx.json")},
            {QStringLiteral("desc"), QStringLiteral("Warm American male voice")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("en_US-amy-medium")},
            {QStringLiteral("name"), QStringLiteral("Amy (American Female)")},
            {QStringLiteral("lang"), QStringLiteral("English (US)")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/amy/medium/en_US-amy-medium.onnx")},
            {QStringLiteral("urlJson"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/amy/medium/en_US-amy-medium.onnx.json")},
            {QStringLiteral("desc"), QStringLiteral("Clear American female voice")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("en_GB-semaine-medium")},
            {QStringLiteral("name"), QStringLiteral("Semaine (British Multi)")},
            {QStringLiteral("lang"), QStringLiteral("English (UK)")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_GB/semaine/medium/en_GB-semaine-medium.onnx")},
            {QStringLiteral("urlJson"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_GB/semaine/medium/en_GB-semaine-medium.onnx.json")},
            {QStringLiteral("desc"), QStringLiteral("Expressive British voice")}
        },
    };

    // Only show downloaded voices
    m_availableTtsVoices.clear();
    const QString voicesDir = jarvisDataDir() + QStringLiteral("/piper-voices");
    QDir().mkpath(voicesDir);
    
    for (auto v : allVoices) {
        auto map = v.toMap();
        const QString filename = map[QStringLiteral("id")].toString() + QStringLiteral(".onnx");
        const QString filePath = voicesDir + QStringLiteral("/") + filename;
        
        if (QFile::exists(filePath)) {
            map[QStringLiteral("downloaded")] = true;
            map[QStringLiteral("active")] = (map[QStringLiteral("id")].toString() == m_currentVoiceName);
            m_availableTtsVoices.append(map);
        }
    }
}

// ─────────────────────────────────────────────
// Fetch More
// ─────────────────────────────────────────────

void JarvisSettings::fetchMoreModels()
{
    // Add extended model list
    const QStringList existingIds = [this]() {
        QStringList ids;
        for (const auto &v : std::as_const(m_availableLlmModels))
            ids << v.toMap()[QStringLiteral("id")].toString();
        return ids;
    }();

    const QVariantList extra = {
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("Llama-3.2-1B-Instruct-Q4_K_M")},
            {QStringLiteral("name"), QStringLiteral("Llama 3.2 1B (Tiny)")},
            {QStringLiteral("size"), QStringLiteral("0.8 GB")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/bartowski/Llama-3.2-1B-Instruct-GGUF/resolve/main/Llama-3.2-1B-Instruct-Q4_K_M.gguf")},
            {QStringLiteral("desc"), QStringLiteral("Meta's compact Llama — fast and capable")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("Llama-3.2-3B-Instruct-Q4_K_M")},
            {QStringLiteral("name"), QStringLiteral("Llama 3.2 3B (Small)")},
            {QStringLiteral("size"), QStringLiteral("2.0 GB")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/bartowski/Llama-3.2-3B-Instruct-GGUF/resolve/main/Llama-3.2-3B-Instruct-Q4_K_M.gguf")},
            {QStringLiteral("desc"), QStringLiteral("Meta's Llama 3.2 — great quality for size")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("gemma-2-2b-it-Q4_K_M")},
            {QStringLiteral("name"), QStringLiteral("Gemma 2 2B (Compact)")},
            {QStringLiteral("size"), QStringLiteral("1.5 GB")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/bartowski/gemma-2-2b-it-GGUF/resolve/main/gemma-2-2b-it-Q4_K_M.gguf")},
            {QStringLiteral("desc"), QStringLiteral("Google's Gemma 2 — efficient and smart")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("gemma-2-9b-it-Q4_K_M")},
            {QStringLiteral("name"), QStringLiteral("Gemma 2 9B (Large)")},
            {QStringLiteral("size"), QStringLiteral("5.8 GB")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/bartowski/gemma-2-9b-it-GGUF/resolve/main/gemma-2-9b-it-Q4_K_M.gguf")},
            {QStringLiteral("desc"), QStringLiteral("Google's Gemma 2 — top quality, needs RAM")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("SmolLM2-1.7B-Instruct-Q4_K_M")},
            {QStringLiteral("name"), QStringLiteral("SmolLM2 1.7B")},
            {QStringLiteral("size"), QStringLiteral("1.1 GB")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/bartowski/SmolLM2-1.7B-Instruct-GGUF/resolve/main/SmolLM2-1.7B-Instruct-Q4_K_M.gguf")},
            {QStringLiteral("desc"), QStringLiteral("HuggingFace's tiny powerhouse")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("TinyLlama-1.1B-Chat-v1.0-Q4_K_M")},
            {QStringLiteral("name"), QStringLiteral("TinyLlama 1.1B Chat")},
            {QStringLiteral("size"), QStringLiteral("0.7 GB")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF/resolve/main/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf")},
            {QStringLiteral("desc"), QStringLiteral("Ultra-lightweight chat model")}
        },
    };

    const QString modelsDir = jarvisDataDir() + QStringLiteral("/models");
    for (const auto &v : extra) {
        auto map = v.toMap();
        if (existingIds.contains(map[QStringLiteral("id")].toString())) continue;
        const QString filename = map[QStringLiteral("id")].toString() + QStringLiteral(".gguf");
        map[QStringLiteral("downloaded")] = QFile::exists(modelsDir + QStringLiteral("/") + filename);
        map[QStringLiteral("active")] = false;
        m_availableLlmModels.append(map);
    }
}

void JarvisSettings::fetchMoreVoices()
{
    const QStringList existingIds = [this]() {
        QStringList ids;
        for (const auto &v : std::as_const(m_availableTtsVoices))
            ids << v.toMap()[QStringLiteral("id")].toString();
        return ids;
    }();

    const QVariantList extra = {
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("en_GB-northern_english_male-medium")},
            {QStringLiteral("name"), QStringLiteral("Northern English Male")},
            {QStringLiteral("lang"), QStringLiteral("English (UK)")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_GB/northern_english_male/medium/en_GB-northern_english_male-medium.onnx")},
            {QStringLiteral("urlJson"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_GB/northern_english_male/medium/en_GB-northern_english_male-medium.onnx.json")},
            {QStringLiteral("desc"), QStringLiteral("Northern English accent — warm tone")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("en_US-lessac-medium")},
            {QStringLiteral("name"), QStringLiteral("Lessac (American)")},
            {QStringLiteral("lang"), QStringLiteral("English (US)")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/lessac/medium/en_US-lessac-medium.onnx")},
            {QStringLiteral("urlJson"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/lessac/medium/en_US-lessac-medium.onnx.json")},
            {QStringLiteral("desc"), QStringLiteral("Professional American voice")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("en_US-libritts_r-medium")},
            {QStringLiteral("name"), QStringLiteral("LibriTTS (American Multi)")},
            {QStringLiteral("lang"), QStringLiteral("English (US)")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/libritts_r/medium/en_US-libritts_r-medium.onnx")},
            {QStringLiteral("urlJson"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/libritts_r/medium/en_US-libritts_r-medium.onnx.json")},
            {QStringLiteral("desc"), QStringLiteral("Multi-speaker American voice")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("en_US-ryan-medium")},
            {QStringLiteral("name"), QStringLiteral("Ryan (American Male)")},
            {QStringLiteral("lang"), QStringLiteral("English (US)")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/ryan/medium/en_US-ryan-medium.onnx")},
            {QStringLiteral("urlJson"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/ryan/medium/en_US-ryan-medium.onnx.json")},
            {QStringLiteral("desc"), QStringLiteral("Confident American male")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("en_GB-jenny_dioco-medium")},
            {QStringLiteral("name"), QStringLiteral("Jenny (British Female)")},
            {QStringLiteral("lang"), QStringLiteral("English (UK)")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_GB/jenny_dioco/medium/en_GB-jenny_dioco-medium.onnx")},
            {QStringLiteral("urlJson"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_GB/jenny_dioco/medium/en_GB-jenny_dioco-medium.onnx.json")},
            {QStringLiteral("desc"), QStringLiteral("Soft-spoken British female")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("de_DE-thorsten-medium")},
            {QStringLiteral("name"), QStringLiteral("Thorsten (German Male)")},
            {QStringLiteral("lang"), QStringLiteral("Deutsch")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/de/de_DE/thorsten/medium/de_DE-thorsten-medium.onnx")},
            {QStringLiteral("urlJson"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/de/de_DE/thorsten/medium/de_DE-thorsten-medium.onnx.json")},
            {QStringLiteral("desc"), QStringLiteral("German male voice")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("fr_FR-siwis-medium")},
            {QStringLiteral("name"), QStringLiteral("Siwis (French)")},
            {QStringLiteral("lang"), QStringLiteral("Français")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/fr/fr_FR/siwis/medium/fr_FR-siwis-medium.onnx")},
            {QStringLiteral("urlJson"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/fr/fr_FR/siwis/medium/fr_FR-siwis-medium.onnx.json")},
            {QStringLiteral("desc"), QStringLiteral("French voice")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("es_ES-davefx-medium")},
            {QStringLiteral("name"), QStringLiteral("DaveFX (Spanish)")},
            {QStringLiteral("lang"), QStringLiteral("Español")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/es/es_ES/davefx/medium/es_ES-davefx-medium.onnx")},
            {QStringLiteral("urlJson"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/es/es_ES/davefx/medium/es_ES-davefx-medium.onnx.json")},
            {QStringLiteral("desc"), QStringLiteral("Spanish male voice")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("pl_PL-gosia-medium")},
            {QStringLiteral("name"), QStringLiteral("Gosia (Polish Female)")},
            {QStringLiteral("lang"), QStringLiteral("Polski")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/pl/pl_PL/gosia/medium/pl_PL-gosia-medium.onnx")},
            {QStringLiteral("urlJson"), QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/main/pl/pl_PL/gosia/medium/pl_PL-gosia-medium.onnx.json")},
            {QStringLiteral("desc"), QStringLiteral("Polish female voice")}
        },
    };

    const QString voicesDir = jarvisDataDir() + QStringLiteral("/piper-voices");
    for (const auto &v : extra) {
        auto map = v.toMap();
        if (existingIds.contains(map[QStringLiteral("id")].toString())) continue;
        const QString filename = map[QStringLiteral("id")].toString() + QStringLiteral(".onnx");
        map[QStringLiteral("downloaded")] = QFile::exists(voicesDir + QStringLiteral("/") + filename);
        map[QStringLiteral("active")] = false;
        m_availableTtsVoices.append(map);
    }
}

// ─────────────────────────────────────────────
// Downloads
// ─────────────────────────────────────────────

void JarvisSettings::downloadLlmModel(const QString &modelId)
{
    if (m_downloading) return;

    QVariantMap targetModel;
    for (const auto &v : std::as_const(m_availableLlmModels)) {
        auto map = v.toMap();
        if (map[QStringLiteral("id")].toString() == modelId) {
            targetModel = map;
            break;
        }
    }
    if (targetModel.isEmpty()) return;

    const QString url = targetModel[QStringLiteral("url")].toString();
    const QString modelsDir = jarvisDataDir() + QStringLiteral("/models");
    QDir().mkpath(modelsDir);
    const QString filePath = modelsDir + QStringLiteral("/") + modelId + QStringLiteral(".gguf");

    if (QFile::exists(filePath)) {
        setActiveLlmModel(modelId);
        return;
    }

    m_downloading = true;
    m_downloadProgress = 0.0;
    m_downloadStatus = QStringLiteral("Downloading ") + targetModel[QStringLiteral("name")].toString() + QStringLiteral("...");
    emit downloadingChanged();
    emit downloadProgressChanged();
    emit downloadStatusChanged();

    QNetworkRequest request{QUrl(url)};
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    m_downloadReply = m_networkManager->get(request);

    auto *outFile = new QFile(filePath + QStringLiteral(".part"), this);
    outFile->open(QIODevice::WriteOnly);

    connect(m_downloadReply, &QNetworkReply::readyRead, this, [this, outFile]() {
        if (outFile->isOpen()) outFile->write(m_downloadReply->readAll());
    });

    connect(m_downloadReply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0) {
            m_downloadProgress = static_cast<double>(received) / total;
            m_downloadStatus = QStringLiteral("Downloading... %1 / %2 MB")
                .arg(received / 1048576).arg(total / 1048576);
            emit downloadProgressChanged();
            emit downloadStatusChanged();
        }
    });

    connect(m_downloadReply, &QNetworkReply::finished, this, [this, outFile, filePath, modelId]() {
        outFile->close();
        outFile->deleteLater();

        if (m_downloadReply->error() == QNetworkReply::NoError) {
            QFile::rename(filePath + QStringLiteral(".part"), filePath);
            m_downloadStatus = QStringLiteral("Download complete!");
            setActiveLlmModel(modelId);
            populateModelList();
        } else {
            QFile::remove(filePath + QStringLiteral(".part"));
            m_downloadStatus = QStringLiteral("Download failed: ") + m_downloadReply->errorString();
        }

        m_downloading = false;
        m_downloadReply = nullptr;
        emit downloadingChanged();
        emit downloadStatusChanged();
    });
}

void JarvisSettings::downloadTtsVoice(const QString &voiceId)
{
    if (m_downloading) return;

    QVariantMap targetVoice;
    for (const auto &v : std::as_const(m_availableTtsVoices)) {
        auto map = v.toMap();
        if (map[QStringLiteral("id")].toString() == voiceId) {
            targetVoice = map;
            break;
        }
    }
    if (targetVoice.isEmpty()) return;

    const QString voicesDir = jarvisDataDir() + QStringLiteral("/piper-voices");
    QDir().mkpath(voicesDir);
    const QString onnxPath = voicesDir + QStringLiteral("/") + voiceId + QStringLiteral(".onnx");
    const QString jsonPath = onnxPath + QStringLiteral(".json");

    if (QFile::exists(onnxPath) && QFile::exists(jsonPath)) {
        setActiveTtsVoice(voiceId);
        return;
    }

    m_downloading = true;
    m_downloadProgress = 0.0;
    m_downloadStatus = QStringLiteral("Downloading voice: ") + targetVoice[QStringLiteral("name")].toString() + QStringLiteral("...");
    emit downloadingChanged();
    emit downloadProgressChanged();
    emit downloadStatusChanged();

    QNetworkRequest request{QUrl(targetVoice[QStringLiteral("url")].toString())};
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    m_downloadReply = m_networkManager->get(request);

    auto *outFile = new QFile(onnxPath + QStringLiteral(".part"), this);
    outFile->open(QIODevice::WriteOnly);

    connect(m_downloadReply, &QNetworkReply::readyRead, this, [this, outFile]() {
        if (outFile->isOpen()) outFile->write(m_downloadReply->readAll());
    });

    connect(m_downloadReply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0) {
            m_downloadProgress = static_cast<double>(received) / total;
            m_downloadStatus = QStringLiteral("Downloading voice... %1 / %2 MB")
                .arg(received / 1048576).arg(total / 1048576);
            emit downloadProgressChanged();
            emit downloadStatusChanged();
        }
    });

    connect(m_downloadReply, &QNetworkReply::finished, this, [this, outFile, onnxPath, jsonPath, targetVoice, voiceId]() {
        outFile->close();
        outFile->deleteLater();

        if (m_downloadReply->error() == QNetworkReply::NoError) {
            QFile::rename(onnxPath + QStringLiteral(".part"), onnxPath);

            QNetworkRequest jsonReq{QUrl(targetVoice[QStringLiteral("urlJson")].toString())};
            jsonReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
            auto *jsonReply = m_networkManager->get(jsonReq);
            connect(jsonReply, &QNetworkReply::finished, this, [this, jsonReply, jsonPath, voiceId]() {
                if (jsonReply->error() == QNetworkReply::NoError) {
                    QFile jf(jsonPath);
                    if (jf.open(QIODevice::WriteOnly)) {
                        jf.write(jsonReply->readAll());
                        jf.close();
                    }
                }
                jsonReply->deleteLater();
                m_downloadStatus = QStringLiteral("Voice downloaded!");
                setActiveTtsVoice(voiceId);
                populateVoiceList();
                m_downloading = false;
                m_downloadReply = nullptr;
                emit downloadingChanged();
                emit downloadStatusChanged();
            });
        } else {
            QFile::remove(onnxPath + QStringLiteral(".part"));
            m_downloadStatus = QStringLiteral("Download failed: ") + m_downloadReply->errorString();
            m_downloading = false;
            m_downloadReply = nullptr;
            emit downloadingChanged();
            emit downloadStatusChanged();
        }
    });
}

void JarvisSettings::setActiveLlmModel(const QString &modelId)
{
    m_currentModelName = modelId;
    saveSettings();
    populateModelList();
    emit currentModelNameChanged();
}

void JarvisSettings::setActiveTtsVoice(const QString &voiceId)
{
    const QString voicesDir = jarvisDataDir() + QStringLiteral("/piper-voices");
    const QString onnxPath = voicesDir + QStringLiteral("/") + voiceId + QStringLiteral(".onnx");

    if (QFile::exists(onnxPath)) {
        m_piperModelPath = onnxPath;
        m_currentVoiceName = voiceId;
        saveSettings();
        populateVoiceList();
        emit currentVoiceNameChanged();
        emit voiceActivated(voiceId, onnxPath);
    }
}

void JarvisSettings::cancelDownload()
{
    if (m_downloadReply) {
        m_downloadReply->abort();
        m_downloadReply = nullptr;
    }
    m_downloading = false;
    m_downloadProgress = 0.0;
    m_downloadStatus = QStringLiteral("Download cancelled.");
    emit downloadingChanged();
    emit downloadProgressChanged();
    emit downloadStatusChanged();
}

// ─────────────────────────────────────────────
// Whisper Model Management
// ─────────────────────────────────────────────

void JarvisSettings::populateWhisperModelList()
{
    m_availableWhisperModels = {
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("ggml-tiny.en")},
            {QStringLiteral("name"), QStringLiteral("Tiny (English only)")},
            {QStringLiteral("size"), QStringLiteral("75 MB")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.en.bin")},
            {QStringLiteral("desc"), QStringLiteral("Fastest, lowest accuracy, English only — ideal for wake word")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("ggml-tiny")},
            {QStringLiteral("name"), QStringLiteral("Tiny (Multilingual)")},
            {QStringLiteral("size"), QStringLiteral("75 MB")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.bin")},
            {QStringLiteral("desc"), QStringLiteral("Fastest, lowest accuracy, supports multiple languages")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("ggml-base.en")},
            {QStringLiteral("name"), QStringLiteral("Base (English only)")},
            {QStringLiteral("size"), QStringLiteral("142 MB")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin")},
            {QStringLiteral("desc"), QStringLiteral("Good balance of speed and accuracy, English only")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("ggml-base")},
            {QStringLiteral("name"), QStringLiteral("Base (Multilingual)")},
            {QStringLiteral("size"), QStringLiteral("142 MB")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin")},
            {QStringLiteral("desc"), QStringLiteral("Good balance, supports multiple languages")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("ggml-small.en")},
            {QStringLiteral("name"), QStringLiteral("Small (English only)")},
            {QStringLiteral("size"), QStringLiteral("466 MB")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.en.bin")},
            {QStringLiteral("desc"), QStringLiteral("High accuracy, English only — slower")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("ggml-small")},
            {QStringLiteral("name"), QStringLiteral("Small (Multilingual)")},
            {QStringLiteral("size"), QStringLiteral("466 MB")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.bin")},
            {QStringLiteral("desc"), QStringLiteral("High accuracy, multilingual — slower")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("ggml-medium.en")},
            {QStringLiteral("name"), QStringLiteral("Medium (English only)")},
            {QStringLiteral("size"), QStringLiteral("1.5 GB")},
            {QStringLiteral("url"), QStringLiteral("https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.en.bin")},
            {QStringLiteral("desc"), QStringLiteral("Very high accuracy, needs more RAM")}
        },
    };

    const QString whisperDir = jarvisDataDir() + QStringLiteral("/whisper-models");
    QDir().mkpath(whisperDir);
    for (auto &v : m_availableWhisperModels) {
        auto map = v.toMap();
        const QString filename = map[QStringLiteral("id")].toString() + QStringLiteral(".bin");
        map[QStringLiteral("downloaded")] = QFile::exists(whisperDir + QStringLiteral("/") + filename);
        map[QStringLiteral("active")] = (map[QStringLiteral("id")].toString() == m_currentWhisperModel);
        v = map;
    }
}

void JarvisSettings::downloadWhisperModel(const QString &modelId)
{
    if (m_downloading) return;

    QVariantMap targetModel;
    for (const auto &v : std::as_const(m_availableWhisperModels)) {
        auto map = v.toMap();
        if (map[QStringLiteral("id")].toString() == modelId) {
            targetModel = map;
            break;
        }
    }
    if (targetModel.isEmpty()) return;

    const QString url = targetModel[QStringLiteral("url")].toString();
    const QString whisperDir = jarvisDataDir() + QStringLiteral("/whisper-models");
    QDir().mkpath(whisperDir);
    const QString filePath = whisperDir + QStringLiteral("/") + modelId + QStringLiteral(".bin");

    if (QFile::exists(filePath)) {
        setActiveWhisperModel(modelId);
        return;
    }

    m_downloading = true;
    m_downloadProgress = 0.0;
    m_downloadStatus = QStringLiteral("Downloading whisper model: ") + targetModel[QStringLiteral("name")].toString() + QStringLiteral("...");
    emit downloadingChanged();
    emit downloadProgressChanged();
    emit downloadStatusChanged();

    QNetworkRequest request{QUrl(url)};
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    m_downloadReply = m_networkManager->get(request);

    auto *outFile = new QFile(filePath + QStringLiteral(".part"), this);
    outFile->open(QIODevice::WriteOnly);

    connect(m_downloadReply, &QNetworkReply::readyRead, this, [this, outFile]() {
        if (outFile->isOpen()) outFile->write(m_downloadReply->readAll());
    });

    connect(m_downloadReply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0) {
            m_downloadProgress = static_cast<double>(received) / total;
            m_downloadStatus = QStringLiteral("Downloading whisper model... %1 / %2 MB")
                .arg(received / 1048576).arg(total / 1048576);
            emit downloadProgressChanged();
            emit downloadStatusChanged();
        }
    });

    connect(m_downloadReply, &QNetworkReply::finished, this, [this, outFile, filePath, modelId]() {
        outFile->close();
        outFile->deleteLater();

        if (m_downloadReply->error() == QNetworkReply::NoError) {
            QFile::rename(filePath + QStringLiteral(".part"), filePath);
            m_downloadStatus = QStringLiteral("Whisper model downloaded!");
            setActiveWhisperModel(modelId);
            populateWhisperModelList();
        } else {
            QFile::remove(filePath + QStringLiteral(".part"));
            m_downloadStatus = QStringLiteral("Download failed: ") + m_downloadReply->errorString();
        }

        m_downloading = false;
        m_downloadReply = nullptr;
        emit downloadingChanged();
        emit downloadStatusChanged();
    });
}

void JarvisSettings::setActiveWhisperModel(const QString &modelId)
{
    const QString whisperDir = jarvisDataDir() + QStringLiteral("/whisper-models");
    const QString modelPath = whisperDir + QStringLiteral("/") + modelId + QStringLiteral(".bin");

    if (QFile::exists(modelPath)) {
        m_whisperModelPath = modelPath;
        m_currentWhisperModel = modelId;
        saveSettings();
        populateWhisperModelList();
        emit currentWhisperModelChanged();
        emit whisperModelActivated(modelPath);
    }
}

// ─────────────────────────────────────────────
// Piper Binary Management
// ─────────────────────────────────────────────

void JarvisSettings::detectPiperBinary()
{
    // Check bundled location first
    const QStringList searchPaths = {
        jarvisDataDir() + QStringLiteral("/piper/piper"),
        QStringLiteral("/usr/libexec/jarvis/piper"),
        QStringLiteral("/usr/lib/piper-tts/bin/piper"),
        QStringLiteral("/usr/bin/piper"),
        QStringLiteral("/usr/local/bin/piper"),
    };

    for (const auto &path : searchPaths) {
        if (QFile::exists(path)) {
            m_piperBinPath = path;
            m_piperInstalled = true;
            emit piperInstalledChanged();
            emit piperBinaryPathChanged();
            return;
        }
    }

    // Check PATH
    const QString systemPiper = QStandardPaths::findExecutable(QStringLiteral("piper"));
    if (!systemPiper.isEmpty()) {
        m_piperBinPath = systemPiper;
        m_piperInstalled = true;
        emit piperInstalledChanged();
        emit piperBinaryPathChanged();
        return;
    }

    m_piperInstalled = false;
    m_piperBinPath.clear();
}

bool JarvisSettings::llmServerBundled() const
{
    // Always return true - llama-server is now built and bundled with the package
    return true;
}

void JarvisSettings::downloadPiperBinary()
{
    if (m_downloading) return;

    const QString piperDir = jarvisDataDir() + QStringLiteral("/piper");
    QDir().mkpath(piperDir);

    const QString piperBin = piperDir + QStringLiteral("/piper");
    if (QFile::exists(piperBin)) {
        m_piperBinPath = piperBin;
        m_piperInstalled = true;
        emit piperInstalledChanged();
        emit piperBinaryPathChanged();
        m_downloadStatus = QStringLiteral("Piper already installed!");
        emit downloadStatusChanged();
        return;
    }

    m_downloading = true;
    m_downloadProgress = 0.0;
    m_downloadStatus = QStringLiteral("Downloading Piper TTS engine...");
    emit downloadingChanged();
    emit downloadProgressChanged();
    emit downloadStatusChanged();

    // Download Piper release tarball for Linux x86_64
    const QString url = QStringLiteral(
        "https://github.com/rhasspy/piper/releases/download/2023.11.14-2/piper_linux_x86_64.tar.gz");

    QNetworkRequest request{QUrl(url)};
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    m_downloadReply = m_networkManager->get(request);

    const QString tarPath = piperDir + QStringLiteral("/piper.tar.gz");
    auto *outFile = new QFile(tarPath, this);
    outFile->open(QIODevice::WriteOnly);

    connect(m_downloadReply, &QNetworkReply::readyRead, this, [this, outFile]() {
        if (outFile->isOpen()) outFile->write(m_downloadReply->readAll());
    });

    connect(m_downloadReply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0) {
            m_downloadProgress = static_cast<double>(received) / total;
            m_downloadStatus = QStringLiteral("Downloading Piper... %1 / %2 MB")
                .arg(received / 1048576).arg(total / 1048576);
            emit downloadProgressChanged();
            emit downloadStatusChanged();
        }
    });

    connect(m_downloadReply, &QNetworkReply::finished, this, [this, outFile, tarPath, piperDir, piperBin]() {
        outFile->close();
        outFile->deleteLater();

        if (m_downloadReply->error() == QNetworkReply::NoError) {
            m_downloadStatus = QStringLiteral("Extracting Piper...");
            emit downloadStatusChanged();

            // Extract tar.gz — piper binary is inside piper/ subdirectory
            auto *extractProc = new QProcess(this);
            connect(extractProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                    this, [this, extractProc, tarPath, piperDir, piperBin](int exitCode, QProcess::ExitStatus) {
                extractProc->deleteLater();
                QFile::remove(tarPath);

                if (exitCode == 0 && QFile::exists(piperBin)) {
                    // Make executable
                    QFile::setPermissions(piperBin,
                        QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
                        QFileDevice::ReadGroup | QFileDevice::ExeGroup |
                        QFileDevice::ReadOther | QFileDevice::ExeOther);

                    m_piperBinPath = piperBin;
                    m_piperInstalled = true;
                    m_downloadStatus = QStringLiteral("Piper installed successfully!");
                    emit piperInstalledChanged();
                    emit piperBinaryPathChanged();
                } else {
                    // The tarball extracts to piper/ subdir, try that path
                    const QString altBin = piperDir + QStringLiteral("/piper/piper");
                    if (QFile::exists(altBin)) {
                        m_piperBinPath = altBin;
                        m_piperInstalled = true;
                        m_downloadStatus = QStringLiteral("Piper installed successfully!");
                        emit piperInstalledChanged();
                        emit piperBinaryPathChanged();
                    } else {
                        m_downloadStatus = QStringLiteral("Failed to extract Piper (exit code %1)").arg(exitCode);
                    }
                }
                m_downloading = false;
                m_downloadReply = nullptr;
                emit downloadingChanged();
                emit downloadStatusChanged();
            });
            extractProc->setWorkingDirectory(piperDir);
            extractProc->start(QStringLiteral("tar"),
                {QStringLiteral("xzf"), tarPath, QStringLiteral("--strip-components=1")});
        } else {
            QFile::remove(tarPath);
            m_downloadStatus = QStringLiteral("Download failed: ") + m_downloadReply->errorString();
            m_downloading = false;
            m_downloadReply = nullptr;
            emit downloadingChanged();
            emit downloadStatusChanged();
        }
    });
}

// ─────────────────────────────────────────────
// Model Management - Delete Functions
// ─────────────────────────────────────────────

void JarvisSettings::deleteLlmModel(const QString &modelId)
{
    const QString modelsDir = jarvisDataDir() + QStringLiteral("/models");
    const QString filePath = modelsDir + QStringLiteral("/") + modelId + QStringLiteral(".gguf");
    
    if (QFile::exists(filePath)) {
        QFile::remove(filePath);
        
        // If this was the active model, clear it
        if (m_currentModelName == modelId) {
            m_currentModelName.clear();
            emit currentModelNameChanged();
        }
        
        // Refresh the model list
        populateModelList();
    }
}

void JarvisSettings::deleteTtsVoice(const QString &voiceId)
{
    const QString voicesDir = jarvisDataDir() + QStringLiteral("/piper-voices");
    const QString onnxPath = voicesDir + QStringLiteral("/") + voiceId + QStringLiteral(".onnx");
    const QString jsonPath = onnxPath + QStringLiteral(".json");
    
    bool deleted = false;
    if (QFile::exists(onnxPath)) {
        QFile::remove(onnxPath);
        deleted = true;
    }
    if (QFile::exists(jsonPath)) {
        QFile::remove(jsonPath);
        deleted = true;
    }
    
    if (deleted) {
        // If this was the active voice, clear it
        if (m_currentVoiceName == voiceId) {
            m_currentVoiceName.clear();
            m_piperModelPath.clear();
            emit currentVoiceNameChanged();
            emit piperBinaryPathChanged();
        }
        
        // Refresh the voice list
        populateVoiceList();
    }
}

void JarvisSettings::deleteWhisperModel(const QString &modelId)
{
    const QString whisperDir = jarvisDataDir() + QStringLiteral("/whisper-models");
    const QString filePath = whisperDir + QStringLiteral("/") + modelId + QStringLiteral(".bin");
    
    if (QFile::exists(filePath)) {
        QFile::remove(filePath);
        
        // If this was the active model, clear it
        if (m_currentWhisperModel == modelId) {
            m_currentWhisperModel.clear();
            m_whisperModelPath.clear();
            emit currentWhisperModelChanged();
        }
        
        // Refresh the whisper model list
        populateWhisperModelList();
    }
}
