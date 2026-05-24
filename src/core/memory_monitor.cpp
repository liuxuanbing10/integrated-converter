#include "memory_monitor.h"
#include "logger.h"
#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#else
#include <sys/sysinfo.h>
#include <fstream>
#endif

MemoryMonitor* MemoryMonitor::instance() {
    static MemoryMonitor s_instance;
    return &s_instance;
}

MemoryMonitor::MemoryMonitor()
    : m_checkTimer(new QTimer(this))
    , m_warningThreshold(0.70)
    , m_criticalThreshold(0.85)
    , m_lastLevel(MemoryLevel::Normal)
    , m_totalSystemMemory(0)
{
    m_totalSystemMemory = getSystemTotalMemory();
    connect(m_checkTimer, &QTimer::timeout, this, &MemoryMonitor::checkMemory);
    LOG_INFO("MemoryMonitor", QString("内存监控器初始化，总内存: %1 MB")
             .arg(m_totalSystemMemory / (1024 * 1024)));
}

MemoryMonitor::~MemoryMonitor() {
    stopMonitoring();
}

void MemoryMonitor::setWarningThreshold(double ratio) {
    QMutexLocker locker(&m_mutex);
    m_warningThreshold = qBound(0.0, ratio, 1.0);
}

void MemoryMonitor::setCriticalThreshold(double ratio) {
    QMutexLocker locker(&m_mutex);
    m_criticalThreshold = qBound(0.0, ratio, 1.0);
}

void MemoryMonitor::setCheckInterval(int ms) {
    m_checkTimer->setInterval(qMax(100, ms));
}

void MemoryMonitor::startMonitoring() {
    if (!m_checkTimer->isActive()) {
        m_checkTimer->start(1000);
        LOG_INFO("MemoryMonitor", "开始内存监控");
    }
}

void MemoryMonitor::stopMonitoring() {
    if (m_checkTimer->isActive()) {
        m_checkTimer->stop();
        LOG_INFO("MemoryMonitor", "停止内存监控");
    }
}

size_t MemoryMonitor::getSystemTotalMemory() const {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) {
        return status.ullTotalPhys;
    }
    return 8ULL * 1024ULL * 1024ULL * 1024ULL;
#else
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return info.totalram * info.mem_unit;
    }
    return 8ULL * 1024ULL * 1024ULL * 1024ULL;
#endif
}

size_t MemoryMonitor::getSystemMemoryUsage() const {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) {
        return status.ullTotalPhys - status.ullAvailPhys;
    }
    return 0;
#else
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    size_t total = 0;
    size_t free = 0;
    size_t buffers = 0;
    size_t cached = 0;
    while (std::getline(meminfo, line)) {
        if (line.find("MemTotal:") == 0) {
            total = std::stoull(line.substr(9)) * 1024;
        } else if (line.find("MemFree:") == 0) {
            free = std::stoull(line.substr(8)) * 1024;
        } else if (line.find("Buffers:") == 0) {
            buffers = std::stoull(line.substr(8)) * 1024;
        } else if (line.find("Cached:") == 0) {
            cached = std::stoull(line.substr(7)) * 1024;
        }
    }
    return total - free - buffers - cached;
#endif
}

size_t MemoryMonitor::currentUsage() const {
    return getSystemMemoryUsage();
}

size_t MemoryMonitor::availableMemory() const {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) {
        return status.ullAvailPhys;
    }
    return 0;
#else
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return info.freeram * info.mem_unit;
    }
    return 0;
#endif
}

size_t MemoryMonitor::totalMemory() const {
    return m_totalSystemMemory;
}

double MemoryMonitor::usageRatio() const {
    size_t total = m_totalSystemMemory;
    if (total == 0) return 0.0;
    return static_cast<double>(currentUsage()) / static_cast<double>(total);
}

bool MemoryMonitor::isUnderPressure() const {
    return usageRatio() >= m_warningThreshold;
}

MemoryMonitor::MemoryLevel MemoryMonitor::currentLevel() const {
    double ratio = usageRatio();
    if (ratio >= m_criticalThreshold) {
        return MemoryLevel::Critical;
    } else if (ratio >= m_warningThreshold) {
        return MemoryLevel::Warning;
    }
    return MemoryLevel::Normal;
}

void MemoryMonitor::checkMemory() {
    size_t used = currentUsage();
    size_t total = m_totalSystemMemory;
    emit memoryUpdated(used, total);
    MemoryLevel level = currentLevel();
    if (level != m_lastLevel) {
        if (level == MemoryLevel::Warning) {
            size_t threshold = static_cast<size_t>(total * m_warningThreshold);
            LOG_WARNING("MemoryMonitor", QString("内存警告: 使用率 %1%")
                       .arg(static_cast<int>(usageRatio() * 100)));
            emit memoryWarning(used, threshold);
        } else if (level == MemoryLevel::Critical) {
            size_t threshold = static_cast<size_t>(total * m_criticalThreshold);
            LOG_ERROR("MemoryMonitor", QString("内存严重警告: 使用率 %1%")
                     .arg(static_cast<int>(usageRatio() * 100)));
            emit memoryCritical(used, threshold);
        } else if (level == MemoryLevel::Normal) {
            LOG_INFO("MemoryMonitor", QString("内存恢复正常: 使用率 %1%")
                    .arg(static_cast<int>(usageRatio() * 100)));
            emit memoryNormalized();
        }
        m_lastLevel = level;
    }
}
