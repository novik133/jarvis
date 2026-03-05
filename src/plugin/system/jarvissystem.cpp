#include "jarvissystem.h"

#include <QFile>
#include <QDateTime>
#include <QTime>

JarvisSystem::JarvisSystem(QObject *parent)
    : QObject(parent)
    , m_systemStatsTimer(new QTimer(this))
    , m_clockTimer(new QTimer(this))
{
    initSystemMonitor();

    connect(m_systemStatsTimer, &QTimer::timeout, this, &JarvisSystem::updateSystemStats);
    m_systemStatsTimer->start(3000);
    updateSystemStats();

    connect(m_clockTimer, &QTimer::timeout, this, &JarvisSystem::updateClock);
    m_clockTimer->start(1000);
    updateClock();
}

void JarvisSystem::initSystemMonitor()
{
    QFile hostnameFile(QStringLiteral("/etc/hostname"));
    if (hostnameFile.open(QIODevice::ReadOnly)) {
        m_hostname = QString::fromUtf8(hostnameFile.readAll()).trimmed();
        hostnameFile.close();
    }

    QFile versionFile(QStringLiteral("/proc/version"));
    if (versionFile.open(QIODevice::ReadOnly)) {
        const QString ver = QString::fromUtf8(versionFile.readAll()).trimmed();
        const auto parts = ver.split(QLatin1Char(' '));
        if (parts.size() >= 3) {
            m_kernelVersion = parts[2];
        }
        versionFile.close();
    }
}

void JarvisSystem::updateSystemStats()
{
    readCpuUsage();
    readMemoryUsage();
    readCpuTemp();
    readUptime();
    emit systemStatsChanged();
}

void JarvisSystem::readCpuUsage()
{
    QFile file(QStringLiteral("/proc/stat"));
    if (!file.open(QIODevice::ReadOnly)) return;

    const QByteArray contents = file.readAll();
    file.close();

    const int nlPos = contents.indexOf('\n');
    const QString line = QString::fromUtf8(nlPos > 0 ? contents.left(nlPos) : contents);

    if (!line.startsWith(QStringLiteral("cpu "))) return;

    const auto parts = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (parts.size() < 8) return;

    long long user   = parts[1].toLongLong();
    long long nice   = parts[2].toLongLong();
    long long system = parts[3].toLongLong();
    long long idle   = parts[4].toLongLong();
    long long iowait = parts[5].toLongLong();
    long long irq    = parts[6].toLongLong();
    long long softirq= parts[7].toLongLong();

    long long total = user + nice + system + idle + iowait + irq + softirq;
    long long totalDiff = total - m_prevCpuTotal;
    long long idleDiff = idle - m_prevCpuIdle;

    if (totalDiff > 0 && m_prevCpuTotal > 0) {
        m_cpuUsage = 100.0 * (1.0 - static_cast<double>(idleDiff) / totalDiff);
    }

    m_prevCpuTotal = total;
    m_prevCpuIdle = idle;
}

void JarvisSystem::readMemoryUsage()
{
    QFile file(QStringLiteral("/proc/meminfo"));
    if (!file.open(QIODevice::ReadOnly)) return;

    const QByteArray contents = file.readAll();
    file.close();

    long long memTotal = 0, memAvailable = 0;

    const auto lines = contents.split('\n');
    for (const QByteArray &rawLine : lines) {
        if (rawLine.startsWith("MemTotal:")) {
            QByteArray val = rawLine.mid(9).trimmed();
            val = val.left(val.indexOf(' '));
            memTotal = val.toLongLong();
        } else if (rawLine.startsWith("MemAvailable:")) {
            QByteArray val = rawLine.mid(13).trimmed();
            val = val.left(val.indexOf(' '));
            memAvailable = val.toLongLong();
        }
        if (memTotal > 0 && memAvailable > 0) break;
    }

    if (memTotal > 0) {
        m_memoryTotalGb = memTotal / 1048576.0;
        m_memoryUsedGb = (memTotal - memAvailable) / 1048576.0;
        m_memoryUsage = 100.0 * static_cast<double>(memTotal - memAvailable) / static_cast<double>(memTotal);
    }
}

void JarvisSystem::readCpuTemp()
{
    const QStringList paths = {
        QStringLiteral("/sys/class/thermal/thermal_zone0/temp"),
        QStringLiteral("/sys/class/hwmon/hwmon0/temp1_input"),
        QStringLiteral("/sys/class/hwmon/hwmon1/temp1_input"),
    };

    for (const auto &path : paths) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            bool ok = false;
            int temp = QString::fromUtf8(file.readAll()).trimmed().toInt(&ok);
            file.close();
            if (ok) {
                m_cpuTemp = (temp > 1000) ? temp / 1000 : temp;
                return;
            }
        }
    }
}

void JarvisSystem::readUptime()
{
    QFile file(QStringLiteral("/proc/uptime"));
    if (!file.open(QIODevice::ReadOnly)) return;

    const QString line = QString::fromUtf8(file.readAll()).trimmed();
    file.close();

    bool ok = false;
    double seconds = line.split(QLatin1Char(' '))[0].toDouble(&ok);
    if (!ok) return;

    int totalSec = static_cast<int>(seconds);
    int days = totalSec / 86400;
    int hours = (totalSec % 86400) / 3600;
    int mins = (totalSec % 3600) / 60;

    if (days > 0) {
        m_uptime = QStringLiteral("%1d %2h %3m").arg(days).arg(hours).arg(mins);
    } else if (hours > 0) {
        m_uptime = QStringLiteral("%1h %2m").arg(hours).arg(mins);
    } else {
        m_uptime = QStringLiteral("%1m").arg(mins);
    }
}

void JarvisSystem::updateClock()
{
    const auto now = QDateTime::currentDateTime();
    m_currentTime = now.toString(QStringLiteral("HH:mm:ss"));
    m_currentDate = now.toString(QStringLiteral("dddd, MMMM d, yyyy"));
    emit currentTimeChanged();
}

QString JarvisSystem::greeting() const
{
    const int hour = QTime::currentTime().hour();
    if (hour < 6)  return QStringLiteral("Good night, Sir.");
    if (hour < 12) return QStringLiteral("Good morning, Sir.");
    if (hour < 18) return QStringLiteral("Good afternoon, Sir.");
    return QStringLiteral("Good evening, Sir.");
}
