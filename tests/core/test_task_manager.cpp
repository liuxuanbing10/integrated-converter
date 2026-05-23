#include <QTest>
#include <QSignalSpy>
#include <QThread>
#include <QEventLoop>
#include "../../src/core/task_manager.h"
#include "../../src/core/conversion_task.h"
#include "../../src/core/iconverter.h"
class MockConverter : public QObject, public IConverter {
    Q_OBJECT
public:
    explicit MockConverter(QObject* parent = nullptr) : QObject(parent) {}
    bool convert(const QString& inputFile, const QString& outputFile,
                const QVariantMap& params) override {
        Q_UNUSED(inputFile);
        Q_UNUSED(outputFile);
        Q_UNUSED(params);
        QThread::msleep(100);
        return true;
    }
    QStringList supportedInputFormats() const override {
        return {"mock_in"};
    }
    QStringList supportedOutputFormats() const override {
        return {"mock_out"};
    }
    QString name() const override { return "MockConverter"; }
    bool isConversionSupported(const QString& inputFormat,
                              const QString& outputFormat) const override {
        return inputFormat == "mock_in" && outputFormat == "mock_out";
    }
};
class TestTaskManager : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        TaskManager::instance()->cancelAllTasks();
    }
    void cleanupTestCase() {
        TaskManager::instance()->cancelAllTasks();
    }
    void init() {
        TaskManager::instance()->cancelAllTasks();
        TaskManager::instance()->setMaxParallelTasks(2);
    }
    void testRegisterConverter() {
        TaskManager* tm = TaskManager::instance();
        auto converter = std::make_shared<MockConverter>();
        tm->registerConverter("MockConverter", converter);
        QVERIFY(tm->availableConverters().contains("MockConverter"));
        QCOMPARE(tm->converter("MockConverter"), converter.get());
        tm->unregisterConverter("MockConverter");
        QVERIFY(!tm->availableConverters().contains("MockConverter"));
    }
    void testAddTask() {
        TaskManager* tm = TaskManager::instance();
        QString taskId = tm->addTask("input.txt", "output.txt", QVariantMap());
        QVERIFY(!taskId.isEmpty());
        ConversionTask* task = tm->getTask(taskId);
        QVERIFY(task != nullptr);
        QCOMPARE(task->inputFile(), QString("input.txt"));
        QCOMPARE(task->outputFile(), QString("output.txt"));
        QCOMPARE(task->status(), ConversionTask::Status::Pending);
        tm->removeTask(taskId);
    }
    void testRemoveTask() {
        TaskManager* tm = TaskManager::instance();
        QString taskId = tm->addTask("input.txt", "output.txt", QVariantMap());
        QVERIFY(tm->getTask(taskId) != nullptr);
        tm->removeTask(taskId);
        QVERIFY(tm->getTask(taskId) == nullptr);
    }
    void testCancelTask() {
        TaskManager* tm = TaskManager::instance();
        QString taskId = tm->addTask("input.txt", "output.txt", QVariantMap());
        ConversionTask* task = tm->getTask(taskId);
        QVERIFY(task != nullptr);
        tm->cancelTask(taskId);
        QCOMPARE(task->status(), ConversionTask::Status::Cancelled);
        QVERIFY(task->isCancelled());
        tm->removeTask(taskId);
    }
    void testCancelAllTasks() {
        TaskManager* tm = TaskManager::instance();
        QString taskId1 = tm->addTask("input1.txt", "output1.txt", QVariantMap());
        QString taskId2 = tm->addTask("input2.txt", "output2.txt", QVariantMap());
        QString taskId3 = tm->addTask("input3.txt", "output3.txt", QVariantMap());
        QCOMPARE(tm->totalTaskCount(), 3);
        tm->cancelAllTasks();
        QCOMPARE(tm->totalTaskCount(), 0);
    }
    void testMaxParallelTasks() {
        TaskManager* tm = TaskManager::instance();
        tm->setMaxParallelTasks(8);
        QCOMPARE(tm->maxParallelTasks(), 8);
        tm->setMaxParallelTasks(1);
        QCOMPARE(tm->maxParallelTasks(), 1);
        tm->setMaxParallelTasks(4);
        QCOMPARE(tm->maxParallelTasks(), 4);
    }
    void testTaskCounts() {
        TaskManager* tm = TaskManager::instance();
        QCOMPARE(tm->totalTaskCount(), 0);
        QCOMPARE(tm->pendingCount(), 0);
        QCOMPARE(tm->runningCount(), 0);
        QCOMPARE(tm->completedCount(), 0);
        QCOMPARE(tm->failedCount(), 0);
        QString taskId1 = tm->addTask("input1.txt", "output1.txt", QVariantMap());
        QString taskId2 = tm->addTask("input2.txt", "output2.txt", QVariantMap());
        QCOMPARE(tm->totalTaskCount(), 2);
        QCOMPARE(tm->pendingCount(), 2);
        tm->removeTask(taskId1);
        tm->removeTask(taskId2);
    }
    void testGetTasksByStatus() {
        TaskManager* tm = TaskManager::instance();
        QString taskId1 = tm->addTask("input1.txt", "output1.txt", QVariantMap());
        QString taskId2 = tm->addTask("input2.txt", "output2.txt", QVariantMap());
        QList<ConversionTask*> pendingTasks = tm->getPendingTasks();
        QCOMPARE(pendingTasks.size(), 2);
        QList<ConversionTask*> runningTasks = tm->getRunningTasks();
        QCOMPARE(runningTasks.size(), 0);
        QList<ConversionTask*> completedTasks = tm->getCompletedTasks();
        QCOMPARE(completedTasks.size(), 0);
        tm->cancelAllTasks();
    }
    void testPauseAndResume() {
        TaskManager* tm = TaskManager::instance();
        QVERIFY(!tm->isPaused());
        tm->pause();
        QVERIFY(tm->isPaused());
        tm->resume();
        QVERIFY(!tm->isPaused());
    }
    void testStart() {
        TaskManager* tm = TaskManager::instance();
        QVERIFY(!tm->isRunning());
        tm->start();
        QVERIFY(tm->isRunning());
        tm->pause();
        tm->resume();
    }
    void testTaskAddedSignal() {
        TaskManager* tm = TaskManager::instance();
        QSignalSpy spy(tm, &TaskManager::taskAdded);
        QString taskId = tm->addTask("input.txt", "output.txt", QVariantMap());
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), taskId);
        tm->removeTask(taskId);
    }
    void testTaskRemovedSignal() {
        TaskManager* tm = TaskManager::instance();
        QString taskId = tm->addTask("input.txt", "output.txt", QVariantMap());
        QSignalSpy spy(tm, &TaskManager::taskRemoved);
        tm->removeTask(taskId);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), taskId);
    }
    void testMemoryPressureThreshold() {
        TaskManager* tm = TaskManager::instance();
        tm->setMemoryPressureThreshold(0.8);
        QVERIFY(!tm->isMemoryUnderPressure());
    }
    void testSingleton() {
        TaskManager* instance1 = TaskManager::instance();
        TaskManager* instance2 = TaskManager::instance();
        QCOMPARE(instance1, instance2);
    }
};
#include "test_task_manager.moc"
