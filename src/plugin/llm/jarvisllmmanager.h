#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QString>
#include <atomic>

class JarvisSettings;

class JarvisLlmManager : public QObject
{
    Q_OBJECT

public:
    explicit JarvisLlmManager(JarvisSettings *settings, QObject *parent = nullptr);
    ~JarvisLlmManager() override;

    [[nodiscard]] bool isServerRunning() const { return m_serverRunning.load(); }
    [[nodiscard]] QString serverUrl() const;
    [[nodiscard]] int serverPort() const { return m_port; }

    void startServer();
    void stopServer();
    void restartServer();
    void loadModel(const QString &modelPath);

signals:
    void serverStarted();
    void serverStopped();
    void serverError(const QString &error);
    void serverRunningChanged();

private slots:
    void onServerStarted();
    void onServerFinished(int exitCode, QProcess::ExitStatus status);
    void onServerReadyReadStdout();
    void onServerReadyReadStderr();

private:
    QString findLlamaServer() const;

    JarvisSettings *m_settings{nullptr};
    QProcess *m_serverProcess{nullptr};
    QTimer *m_startupTimer{nullptr};
    int m_port{8080};
    std::atomic<bool> m_serverRunning{false};
    QString m_currentModelPath;
    bool m_intentionalStop{false};
};
