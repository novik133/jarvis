#include "jarviscommands.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

JarvisCommands::JarvisCommands(QObject *parent)
    : QObject(parent)
{
    loadCommands();
    if (m_commandMappings.isEmpty()) {
        populateDefaults();
    }
}

// ─────────────────────────────────────────────
// Defaults
// ─────────────────────────────────────────────

void JarvisCommands::populateDefaults()
{
    m_commandMappings = {
        QVariantMap{
            {QStringLiteral("phrase"), QStringLiteral("open terminal")},
            {QStringLiteral("action"), QStringLiteral("x-terminal-emulator||konsole||gnome-terminal||xterm")},
            {QStringLiteral("type"), QStringLiteral("app")},
            {QStringLiteral("desc"), QStringLiteral("Open default terminal emulator")}
        },
        QVariantMap{
            {QStringLiteral("phrase"), QStringLiteral("open console")},
            {QStringLiteral("action"), QStringLiteral("x-terminal-emulator||konsole||gnome-terminal||xterm")},
            {QStringLiteral("type"), QStringLiteral("app")},
            {QStringLiteral("desc"), QStringLiteral("Open default terminal emulator")}
        },
        QVariantMap{
            {QStringLiteral("phrase"), QStringLiteral("open file manager")},
            {QStringLiteral("action"), QStringLiteral("xdg-open ~||dolphin||nautilus||thunar")},
            {QStringLiteral("type"), QStringLiteral("app")},
            {QStringLiteral("desc"), QStringLiteral("Open default file manager")}
        },
        QVariantMap{
            {QStringLiteral("phrase"), QStringLiteral("open files")},
            {QStringLiteral("action"), QStringLiteral("xdg-open ~||dolphin||nautilus||thunar")},
            {QStringLiteral("type"), QStringLiteral("app")},
            {QStringLiteral("desc"), QStringLiteral("Open default file manager")}
        },
        QVariantMap{
            {QStringLiteral("phrase"), QStringLiteral("open browser")},
            {QStringLiteral("action"), QStringLiteral("xdg-open https://||firefox||chromium||google-chrome")},
            {QStringLiteral("type"), QStringLiteral("app")},
            {QStringLiteral("desc"), QStringLiteral("Open default web browser")}
        },
        QVariantMap{
            {QStringLiteral("phrase"), QStringLiteral("open settings")},
            {QStringLiteral("action"), QStringLiteral("systemsettings||gnome-control-center")},
            {QStringLiteral("type"), QStringLiteral("app")},
            {QStringLiteral("desc"), QStringLiteral("Open system settings")}
        },
        QVariantMap{
            {QStringLiteral("phrase"), QStringLiteral("open system monitor")},
            {QStringLiteral("action"), QStringLiteral("plasma-systemmonitor||ksysguard||gnome-system-monitor||htop")},
            {QStringLiteral("type"), QStringLiteral("app")},
            {QStringLiteral("desc"), QStringLiteral("Open system monitor")}
        },
        QVariantMap{
            {QStringLiteral("phrase"), QStringLiteral("open text editor")},
            {QStringLiteral("action"), QStringLiteral("kate||kwrite||gedit||xed||nano")},
            {QStringLiteral("type"), QStringLiteral("app")},
            {QStringLiteral("desc"), QStringLiteral("Open text editor")}
        },
        QVariantMap{
            {QStringLiteral("phrase"), QStringLiteral("open calculator")},
            {QStringLiteral("action"), QStringLiteral("kcalc||gnome-calculator||galculator")},
            {QStringLiteral("type"), QStringLiteral("app")},
            {QStringLiteral("desc"), QStringLiteral("Open calculator")}
        },
        QVariantMap{
            {QStringLiteral("phrase"), QStringLiteral("lock screen")},
            {QStringLiteral("action"), QStringLiteral("loginctl lock-session")},
            {QStringLiteral("type"), QStringLiteral("command")},
            {QStringLiteral("desc"), QStringLiteral("Lock the screen")}
        },
        QVariantMap{
            {QStringLiteral("phrase"), QStringLiteral("take screenshot")},
            {QStringLiteral("action"), QStringLiteral("spectacle||gnome-screenshot||scrot")},
            {QStringLiteral("type"), QStringLiteral("app")},
            {QStringLiteral("desc"), QStringLiteral("Take a screenshot")}
        },
        QVariantMap{
            {QStringLiteral("phrase"), QStringLiteral("increase volume")},
            {QStringLiteral("action"), QStringLiteral("wpctl set-volume @DEFAULT_AUDIO_SINK@ 10%+")},
            {QStringLiteral("type"), QStringLiteral("command")},
            {QStringLiteral("desc"), QStringLiteral("Increase system volume by 10%")}
        },
        QVariantMap{
            {QStringLiteral("phrase"), QStringLiteral("decrease volume")},
            {QStringLiteral("action"), QStringLiteral("wpctl set-volume @DEFAULT_AUDIO_SINK@ 10%-")},
            {QStringLiteral("type"), QStringLiteral("command")},
            {QStringLiteral("desc"), QStringLiteral("Decrease system volume by 10%")}
        },
        QVariantMap{
            {QStringLiteral("phrase"), QStringLiteral("mute")},
            {QStringLiteral("action"), QStringLiteral("wpctl set-mute @DEFAULT_AUDIO_SINK@ toggle")},
            {QStringLiteral("type"), QStringLiteral("command")},
            {QStringLiteral("desc"), QStringLiteral("Toggle mute")}
        },
    };
    saveCommands();
}

// ─────────────────────────────────────────────
// Matching
// ─────────────────────────────────────────────

bool JarvisCommands::tryExecuteVoiceCommand(const QString &spokenText)
{
    const QString lower = spokenText.toLower().trimmed();

    for (const auto &v : std::as_const(m_commandMappings)) {
        const auto map = v.toMap();
        const QString phrase = map[QStringLiteral("phrase")].toString().toLower();
        const QString action = map[QStringLiteral("action")].toString();
        const QString type = map[QStringLiteral("type")].toString();

        if (!lower.contains(phrase)) continue;

        qDebug() << "[JARVIS] Voice command matched:" << phrase << "->" << action;

        if (type == QStringLiteral("command")) {
            // Direct shell command
            auto *proc = new QProcess(this);
            connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                    this, [this, proc, phrase](int exitCode, QProcess::ExitStatus) {
                const QString out = QString::fromUtf8(proc->readAllStandardOutput()).trimmed();
                if (!out.isEmpty()) emit commandOutput(out);
                if (exitCode != 0) {
                    const QString err = QString::fromUtf8(proc->readAllStandardError()).trimmed();
                    if (!err.isEmpty()) emit commandOutput(QStringLiteral("Error: ") + err);
                }
                proc->deleteLater();
            });
            proc->start(QStringLiteral("/bin/sh"), {QStringLiteral("-c"), action});
            emit commandExecuted(phrase, action);
            return true;
        }

        if (type == QStringLiteral("app")) {
            // Try each alternative separated by ||
            const auto alternatives = action.split(QStringLiteral("||"));
            for (const auto &alt : alternatives) {
                const QString cmd = alt.trimmed();
                if (cmd.isEmpty()) continue;

                // Check if it's a command with args (like "xdg-open ~")
                const auto parts = cmd.split(QLatin1Char(' '));
                const QString bin = parts.first();

                // Try to find the binary
                auto *which = new QProcess(this);
                which->start(QStringLiteral("which"), {bin});
                which->waitForFinished(1000);
                if (which->exitCode() == 0) {
                    delete which;
                    QProcess::startDetached(QStringLiteral("/bin/sh"), {QStringLiteral("-c"), cmd});
                    emit commandExecuted(phrase, cmd);
                    return true;
                }
                delete which;
            }
            qDebug() << "[JARVIS] No suitable binary found for:" << action;
        }

        return false;
    }

    return false;
}

// ─────────────────────────────────────────────
// Management
// ─────────────────────────────────────────────

void JarvisCommands::addCommand(const QString &phrase, const QString &action, const QString &type)
{
    QVariantMap map;
    map[QStringLiteral("phrase")] = phrase;
    map[QStringLiteral("action")] = action;
    map[QStringLiteral("type")] = type;
    map[QStringLiteral("desc")] = QStringLiteral("User-defined command");
    m_commandMappings.append(map);
    saveCommands();
    emit commandMappingsChanged();
}

void JarvisCommands::removeCommand(int index)
{
    if (index < 0 || index >= m_commandMappings.size()) return;
    m_commandMappings.removeAt(index);
    saveCommands();
    emit commandMappingsChanged();
}

void JarvisCommands::updateCommand(int index, const QString &phrase, const QString &action, const QString &type)
{
    if (index < 0 || index >= m_commandMappings.size()) return;
    auto map = m_commandMappings[index].toMap();
    map[QStringLiteral("phrase")] = phrase;
    map[QStringLiteral("action")] = action;
    map[QStringLiteral("type")] = type;
    m_commandMappings[index] = map;
    saveCommands();
    emit commandMappingsChanged();
}

void JarvisCommands::resetToDefaults()
{
    populateDefaults();
    emit commandMappingsChanged();
}

// ─────────────────────────────────────────────
// Persistence
// ─────────────────────────────────────────────

void JarvisCommands::loadCommands()
{
    const QString json = m_settings.value(QStringLiteral("commands/mappings")).toString();
    if (json.isEmpty()) return;

    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isArray()) return;

    m_commandMappings.clear();
    const QJsonArray arr = doc.array();
    for (const auto &val : arr) {
        const QJsonObject obj = val.toObject();
        QVariantMap map;
        map[QStringLiteral("phrase")] = obj[QStringLiteral("phrase")].toString();
        map[QStringLiteral("action")] = obj[QStringLiteral("action")].toString();
        map[QStringLiteral("type")] = obj[QStringLiteral("type")].toString();
        map[QStringLiteral("desc")] = obj[QStringLiteral("desc")].toString();
        m_commandMappings.append(map);
    }
}

void JarvisCommands::saveCommands()
{
    QJsonArray arr;
    for (const auto &v : std::as_const(m_commandMappings)) {
        const auto map = v.toMap();
        QJsonObject obj;
        obj[QStringLiteral("phrase")] = map[QStringLiteral("phrase")].toString();
        obj[QStringLiteral("action")] = map[QStringLiteral("action")].toString();
        obj[QStringLiteral("type")] = map[QStringLiteral("type")].toString();
        obj[QStringLiteral("desc")] = map[QStringLiteral("desc")].toString();
        arr.append(obj);
    }
    m_settings.setValue(QStringLiteral("commands/mappings"),
                        QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
    m_settings.sync();
}
