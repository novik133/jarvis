#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QSettings>
#include <QProcess>

class JarvisCommands : public QObject
{
    Q_OBJECT

public:
    explicit JarvisCommands(QObject *parent = nullptr);

    [[nodiscard]] QVariantList commandMappings() const { return m_commandMappings; }

    // Try to match a spoken phrase to a command. Returns true if handled.
    bool tryExecuteVoiceCommand(const QString &spokenText);

    // Management
    void addCommand(const QString &phrase, const QString &action, const QString &type);
    void removeCommand(int index);
    void updateCommand(int index, const QString &phrase, const QString &action, const QString &type);
    void resetToDefaults();

signals:
    void commandMappingsChanged();
    void commandExecuted(const QString &phrase, const QString &action);
    void commandOutput(const QString &output);

private:
    void loadCommands();
    void saveCommands();
    void populateDefaults();

    QSettings m_settings{QStringLiteral("jarvis-plasmoid"), QStringLiteral("jarvis")};
    QVariantList m_commandMappings;
};
