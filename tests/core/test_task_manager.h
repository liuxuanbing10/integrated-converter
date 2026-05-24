#ifndef TEST_TASK_MANAGER_H
#define TEST_TASK_MANAGER_H

#include <QObject>

class TestTaskManager : public QObject {
    Q_OBJECT
public:
    explicit TestTaskManager(QObject* parent = nullptr) : QObject(parent) {}
private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void testRegisterConverter();
    void testAddTask();
    void testRemoveTask();
    void testCancelTask();
    void testCancelAllTasks();
    void testMaxParallelTasks();
    void testTaskCounts();
    void testGetTasksByStatus();
    void testPauseAndResume();
    void testStart();
    void testTaskAddedSignal();
    void testTaskRemovedSignal();
    void testMemoryPressureThreshold();
    void testSingleton();
};

#endif
