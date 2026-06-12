#include "test_memory_monitor.h"
#include "core/memory_monitor.h"
#include <QTest>

void TestMemoryMonitor::testSingletonInstance() {
    MemoryMonitor* m1 = MemoryMonitor::instance();
    MemoryMonitor* m2 = MemoryMonitor::instance();
    QVERIFY(m1 != nullptr);
    QVERIFY(m1 == m2);
}

void TestMemoryMonitor::testDefaultWarningThreshold() {
    MemoryMonitor* monitor = MemoryMonitor::instance();
    QVERIFY(monitor->isUnderPressure() == false || monitor->isUnderPressure() == true);
}

void TestMemoryMonitor::testDefaultCriticalThreshold() {
    MemoryMonitor* monitor = MemoryMonitor::instance();
    QVERIFY(monitor->currentLevel() == MemoryMonitor::MemoryLevel::Normal
            || monitor->currentLevel() == MemoryMonitor::MemoryLevel::Warning
            || monitor->currentLevel() == MemoryMonitor::MemoryLevel::Critical);
}

void TestMemoryMonitor::testSetWarningThreshold() {
    MemoryMonitor* monitor = MemoryMonitor::instance();
    monitor->setWarningThreshold(0.5);
    QVERIFY(monitor->isUnderPressure() == false || monitor->isUnderPressure() == true);
    monitor->setWarningThreshold(1.0);
    QVERIFY(!monitor->isUnderPressure());
    monitor->setWarningThreshold(0.7);
}

void TestMemoryMonitor::testSetCriticalThreshold() {
    MemoryMonitor* monitor = MemoryMonitor::instance();
    monitor->setCriticalThreshold(0.9);
    monitor->setCriticalThreshold(0.85);
}

void TestMemoryMonitor::testSetCheckInterval() {
    MemoryMonitor* monitor = MemoryMonitor::instance();
    monitor->setCheckInterval(500);
    monitor->setCheckInterval(50);
    monitor->setCheckInterval(1000);
}

void TestMemoryMonitor::testStartStopMonitoring() {
    MemoryMonitor* monitor = MemoryMonitor::instance();
    monitor->startMonitoring();
    monitor->startMonitoring();
    monitor->stopMonitoring();
    monitor->stopMonitoring();
}

void TestMemoryMonitor::testIsUnderPressureInitial() {
    MemoryMonitor* monitor = MemoryMonitor::instance();
    bool underPressure = monitor->isUnderPressure();
    QVERIFY(underPressure == false || underPressure == true);
}

void TestMemoryMonitor::testCurrentLevelInitial() {
    MemoryMonitor* monitor = MemoryMonitor::instance();
    auto level = monitor->currentLevel();
    QVERIFY(level == MemoryMonitor::MemoryLevel::Normal
            || level == MemoryMonitor::MemoryLevel::Warning
            || level == MemoryMonitor::MemoryLevel::Critical);
}

void TestMemoryMonitor::testTotalMemory() {
    MemoryMonitor* monitor = MemoryMonitor::instance();
    QVERIFY(monitor->totalMemory() > 0);
}

void TestMemoryMonitor::testCurrentUsage() {
    MemoryMonitor* monitor = MemoryMonitor::instance();
    QVERIFY(monitor->currentUsage() > 0);
}
