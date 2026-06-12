#ifndef TEST_MEMORY_MONITOR_H
#define TEST_MEMORY_MONITOR_H

#include <QObject>
#include <QString>

class TestMemoryMonitor : public QObject {
    Q_OBJECT
public:
    explicit TestMemoryMonitor(QObject* parent = nullptr) : QObject(parent) {}
private slots:
    void testSingletonInstance();
    void testDefaultWarningThreshold();
    void testDefaultCriticalThreshold();
    void testSetWarningThreshold();
    void testSetCriticalThreshold();
    void testSetCheckInterval();
    void testStartStopMonitoring();
    void testIsUnderPressureInitial();
    void testCurrentLevelInitial();
    void testTotalMemory();
    void testCurrentUsage();
};
#endif // TEST_MEMORY_MONITOR_H
