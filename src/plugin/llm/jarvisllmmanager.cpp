#include "jarvisllmmanager.h"
#include "../settings/jarvissettings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDebug>

JarvisLlmManager::JarvisLlmManager(JarvisSettings *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_startupTimer(new QTimer(this))
{
    m_startupTimer->setSingleShot(true);
    m_startupTimer->setInterval(10000); // 10s startup timeout
    connect(m_startupTimer, &QTimer::timeout, this, [this]() {
        if (!m_serverRunning.load()) {
            qWarning() << "[JARVIS] LLM server startup timed out";
            emit serverError(QStringLiteral("LLM server startup timed out"));
        }
    });
}

JarvisLlmManager::~JarvisLlmManager()
{
    stopServer();
}

QString JarvisLlmManager::serverUrl() const
{
    return QStringLiteral("http://127.0.0.1:%1").arg(m_port);
}

QString JarvisLlmManager::findLlamaServer() const
{
    // 1. Bundled binary (installed by CMake)
    const QStringList searchPaths = {
        QStringLiteral("/usr/libexec/jarvis/llama-server"),
        QStringLiteral("/usr/local/libexec/jarvis/llama-server"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/../libexec/jarvis/llama-server"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/llama-server"),
        m_settings->jarvisDataDir() + QStringLiteral("/bin/llama-server"),
    };

    for (const auto &path : searchPaths) {
        if (QFile::exists(path)) {
            qDebug() << "[JARVIS] Found bundled llama-server at:" << path;
            return path;
        }
    }

    // 2. Fallback: system-installed llama-server
    const QString systemPath = QStandardPaths::findExecutable(QStringLiteral("llama-server"));
    if (!systemPath.isEmpty()) {
        qDebug() << "[JARVIS] Found system llama-server at:" << systemPath;
        return systemPath;
    }

    // 3. Fallback: legacy llama.cpp server binary name
    const QString legacyPath = QStandardPaths::findExecutable(QStringLiteral("llama-server"));
    if (!legacyPath.isEmpty()) return legacyPath;

    return {};
}

void JarvisLlmManager::startServer()
{
    if (m_serverProcess && m_serverProcess->state() == QProcess::Running) {
        qDebug() << "[JARVIS] LLM server already running";
        return;
    }

    const QString serverBin = findLlamaServer();
    if (serverBin.isEmpty()) {
        qWarning() << "[JARVIS] llama-server binary not found";
        emit serverError(QStringLiteral("llama-server binary not found. Build and install the project first."));
        return;
    }

    // Find the active model
    const QString modelsDir = m_settings->jarvisDataDir() + QStringLiteral("/models");
    const QString modelName = m_settings->currentModelName();
    QString modelPath;

    if (!modelName.isEmpty()) {
        modelPath = modelsDir + QStringLiteral("/") + modelName + QStringLiteral(".gguf");
        if (!QFile::exists(modelPath)) {
            // Try without .gguf suffix (in case it already has it)
            modelPath = modelsDir + QStringLiteral("/") + modelName;
            if (!QFile::exists(modelPath)) {
                qWarning() << "[JARVIS] Model not found:" << modelPath;
                emit serverError(QStringLiteral("Model not found: %1. Download a model first.").arg(modelName));
                return;
            }
        }
    } else {
        emit serverError(QStringLiteral("No model selected. Download and select a model in settings."));
        return;
    }

    m_currentModelPath = modelPath;

    stopServer();

    m_serverProcess = new QProcess(this);
    m_serverProcess->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_serverProcess, &QProcess::started, this, &JarvisLlmManager::onServerStarted);
    connect(m_serverProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &JarvisLlmManager::onServerFinished);
    connect(m_serverProcess, &QProcess::readyReadStandardOutput,
            this, &JarvisLlmManager::onServerReadyReadStdout);
    connect(m_serverProcess, &QProcess::readyReadStandardError,
            this, &JarvisLlmManager::onServerReadyReadStderr);

    QStringList args;
    args << QStringLiteral("-m") << modelPath;
    args << QStringLiteral("--port") << QString::number(m_port);
    args << QStringLiteral("--host") << QStringLiteral("127.0.0.1");
    args << QStringLiteral("-c") << QStringLiteral("4096");
    args << QStringLiteral("-ngl") << QStringLiteral("0"); // CPU-only by default

    qDebug() << "[JARVIS] Starting llama-server:" << serverBin << args;

    m_intentionalStop = false;
    m_serverProcess->start(serverBin, args);
    m_startupTimer->start();
}

void JarvisLlmManager::stopServer()
{
    m_startupTimer->stop();
    m_intentionalStop = true;

    if (m_serverProcess) {
        if (m_serverProcess->state() != QProcess::NotRunning) {
            m_serverProcess->terminate();
            if (!m_serverProcess->waitForFinished(3000)) {
                m_serverProcess->kill();
                m_serverProcess->waitForFinished(1000);
            }
        }
        delete m_serverProcess;
        m_serverProcess = nullptr;
    }

    if (m_serverRunning.load()) {
        m_serverRunning = false;
        emit serverRunningChanged();
        emit serverStopped();
    }
}

void JarvisLlmManager::restartServer()
{
    stopServer();
    startServer();
}

void JarvisLlmManager::loadModel(const QString &modelPath)
{
    m_currentModelPath = modelPath;
    restartServer();
}

void JarvisLlmManager::onServerStarted()
{
    qDebug() << "[JARVIS] llama-server process started, waiting for ready...";
}

void JarvisLlmManager::onServerFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(status)
    m_startupTimer->stop();

    const bool wasRunning = m_serverRunning.load();
    m_serverRunning = false;

    if (wasRunning) {
        emit serverRunningChanged();
        emit serverStopped();
    }

    if (!m_intentionalStop) {
        qWarning() << "[JARVIS] llama-server exited unexpectedly, code:" << exitCode;
        emit serverError(QStringLiteral("LLM server stopped unexpectedly (exit code %1)").arg(exitCode));
    }
}

void JarvisLlmManager::onServerReadyReadStdout()
{
    if (!m_serverProcess) return;
    const QString output = QString::fromUtf8(m_serverProcess->readAllStandardOutput());

    // llama-server prints "listening" or similar when ready
    if (!m_serverRunning.load() &&
        (output.contains(QStringLiteral("listening"), Qt::CaseInsensitive) ||
         output.contains(QStringLiteral("server is listening"), Qt::CaseInsensitive) ||
         output.contains(QStringLiteral("HTTP server listening"), Qt::CaseInsensitive))) {
        m_startupTimer->stop();
        m_serverRunning = true;
        emit serverRunningChanged();
        emit serverStarted();
        qDebug() << "[JARVIS] LLM server is ready on port" << m_port;
    }
}

void JarvisLlmManager::onServerReadyReadStderr()
{
    if (!m_serverProcess) return;
    const QString output = QString::fromUtf8(m_serverProcess->readAllStandardError());

    // llama.cpp server also logs to stderr
    if (!m_serverRunning.load() &&
        (output.contains(QStringLiteral("listening"), Qt::CaseInsensitive) ||
         output.contains(QStringLiteral("server is listening"), Qt::CaseInsensitive) ||
         output.contains(QStringLiteral("HTTP server listening"), Qt::CaseInsensitive))) {
        m_startupTimer->stop();
        m_serverRunning = true;
        emit serverRunningChanged();
        emit serverStarted();
        qDebug() << "[JARVIS] LLM server is ready on port" << m_port;
    }

    // Check for fatal errors
    if (output.contains(QStringLiteral("error"), Qt::CaseInsensitive) &&
        output.contains(QStringLiteral("failed"), Qt::CaseInsensitive)) {
        qWarning() << "[JARVIS] LLM server error:" << output.trimmed();
    }
}
