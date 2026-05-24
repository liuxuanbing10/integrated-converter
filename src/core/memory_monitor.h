#ifndef MEMORY_MONITOR_H
#define MEMORY_MONITOR_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QAtomicInt>

class MemoryMonitor : public QObject {
    Q_OBJECT

public:
    enum class MemoryLevel {
        Normal,
        Warning,
        Critical
    };

    static MemoryMonitor* instance();
    void setWarningThreshold(double ratio);
    void setCriticalThreshold(double ratio);
    void setCheckInterval(int ms);
    void startMonitoring();
    void stopMonitoring();
    size_t currentUsage() const;
    size_t availableMemory() const;
    size_t totalMemory() const;
    double usageRatio() const;
    bool isUnderPressure() const;
    MemoryLevel currentLevel() const;


signals:
    void memoryWarning(size_t current, size_t threshold);
    void memoryCritical(size_t current, size_t threshold);
    void memoryNormalized();
    void memoryUpdated(size_t used, size_t total);

private slots:
    void checkMemory();

private:
    size_t getSystemMemoryUsage() const;
    size_t getSystemTotalMemory() const;

    QTimer* m_checkTimer;
    mutable QMutex m_mutex;
    double m_warningThreshold;
    double m_criticalThreshold;
    MemoryLevel m_lastLevel;
    size_t m_totalSystemMemory;

    MemoryMonitor();
    ~MemoryMonitor();
    MemoryMonitor(const MemoryMonitor&) = delete;
    MemoryMonitor& operator=(const MemoryMonitor&) = delete;
};
#endif // MEMORY_MONITOR_H
