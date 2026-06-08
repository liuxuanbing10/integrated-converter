#include <QTest>
#include <QSignalSpy>
#include <QThread>
#include <QEventLoop>
#include "../../src/core/task_manager.h"
#include "../../src/core/conversion_task.h"
#include "../../src/core/iconverter.h"
#include "test_task_manager.h"

// MockConverter: test mock, keep inline with Q_OBJECT (no separate header needed)
class MockConverter : public QObject, public IConverter {
    Q_OBJECT
public:
    explicit MockConverter(QObject* parent = nullptr) : QObject(parent) {}
    QString name() const override { return "MockConverter"; }
    QStringList supportedInputFormats() const override { return {"mock_in"}; }
    QStringList supportedOutputFormats() const override { return {"mock_out"}; }
    std::optional<ErrorInfo> convert(const QString& inputFile, const QString& outputFile,
                                     const QVariantMap& params) override {
        Q_UNUSED(inputFile);
        Q_UNUSED(outputFile);
        Q_UNUSED(params);
        return std::nullopt;
    }
    bool isConversionSupported(const QString& inputFormat,
                              const QString& outputFormat) const override {
        return inputFormat == "mock_in" && outputFormat == "mock_out";
    }
};

void TestTaskManager::initTestCase() {
    TaskManager::instance()->cancelAllTasks();
}

void TestTaskManager::cleanupTestCase() {
    TaskManager::instance()->cancelAllTasks();
}

void TestTaskManager::init() {
    TaskManager::instance()->cancelAllTasks();
    TaskManager::instance()->registerConverter("mock", std::make_shared<MockConverter>());
}

void TestTaskManager::testRegisterConverter() {
    auto* tm = TaskManager::instance();
    auto* conv = tm->converter("mock");
    QVERIFY(conv != nullptr);
    QCOMPARE(conv->name(), QString("MockConverter"));
}

void TestTaskManager::testAddTask() {
    auto* tm = TaskManager::instance();
    int before = tm->totalTaskCount();
    QString taskId = tm->addTask("/input.mock_in", "/output.mock_out", QVariantMap());
    QVERIFY(!taskId.isEmpty());
    QCOMPARE(tm->totalTaskCount(), before + 1);
}

void TestTaskManager::testRemoveTask() {
    auto* tm = TaskManager::instance();
    int before = tm->totalTaskCount();
    QString taskId = tm->addTask("/input.mock_in", "/output.mock_out", QVariantMap());
    QVERIFY(!taskId.isEmpty());
    tm->removeTask(taskId);
    QCOMPARE(tm->totalTaskCount(), before);
}

void TestTaskManager::testCancelTask() {
    auto* tm = TaskManager::instance();
    QString taskId = tm->addTask("/input.mock_in", "/output.mock_out", QVariantMap());
    QVERIFY(!taskId.isEmpty());
    tm->cancelTask(taskId);
    auto* task = tm->getTask(taskId);
    QVERIFY(task != nullptr);
    QVERIFY(task->status() == ConversionTask::Status::Cancelled);
}

void TestTaskManager::testCancelAllTasks() {
    auto* tm = TaskManager::instance();
    tm->cancelAllTasks();
    for (int i = 0; i < 5; ++i) {
        tm->addTask("/input.mock_in", "/output.mock_out", QVariantMap());
    }
    tm->cancelAllTasks();
    QCOMPARE(tm->pendingCount(), 0);
    QCOMPARE(tm->runningCount(), 0);
}

void TestTaskManager::testMaxParallelTasks() {
    auto* tm = TaskManager::instance();
    int original = tm->maxParallelTasks();
    tm->setMaxParallelTasks(10);
    QCOMPARE(tm->maxParallelTasks(), 10);
    tm->setMaxParallelTasks(original);
    QCOMPARE(tm->maxParallelTasks(), original);
}

void TestTaskManager::testTaskCounts() {
    auto* tm = TaskManager::instance();
    int before = tm->totalTaskCount();
    for (int i = 0; i < 3; ++i) {
        tm->addTask("/input.mock_in", "/output.mock_out", QVariantMap());
    }
    QCOMPARE(tm->totalTaskCount(), before + 3);
}

void TestTaskManager::testGetTasksByStatus() {
    auto* tm = TaskManager::instance();
    tm->cancelAllTasks();
    tm->addTask("/input.mock_in", "/output.mock_out", QVariantMap());
    QVERIFY(tm->pendingCount() > 0);
}

void TestTaskManager::testPauseAndResume() {
    auto* tm = TaskManager::instance();
    tm->addTask("/input.mock_in", "/output.mock_out", QVariantMap());
    tm->pause();
    QVERIFY(tm->isPaused());
    tm->resume();
    QVERIFY(!tm->isPaused());
}

void TestTaskManager::testStart() {
    auto* tm = TaskManager::instance();
    int originalMax = tm->maxParallelTasks();
    tm->setMaxParallelTasks(4);
    tm->addTask("/input.mock_in", "/output.mock_out", QVariantMap());
    tm->start();
    QVERIFY(tm->totalTaskCount() > 0);
    tm->cancelAllTasks();
    tm->setMaxParallelTasks(originalMax);
}

void TestTaskManager::testTaskAddedSignal() {
    auto* tm = TaskManager::instance();
    QSignalSpy spy(tm, &TaskManager::taskAdded);
    tm->addTask("/input.mock_in", "/output.mock_out", QVariantMap());
    QCOMPARE(spy.count(), 1);
}

void TestTaskManager::testTaskRemovedSignal() {
    auto* tm = TaskManager::instance();
    QString taskId = tm->addTask("/input.mock_in", "/output.mock_out", QVariantMap());
    QSignalSpy spy(tm, &TaskManager::taskRemoved);
    tm->removeTask(taskId);
    QCOMPARE(spy.count(), 1);
}

void TestTaskManager::testMemoryPressureThreshold() {
    auto* tm = TaskManager::instance();
    tm->setMemoryPressureThreshold(0.5);
    QVERIFY(!tm->isMemoryUnderPressure());
}

void TestTaskManager::testSingleton() {
    auto* instance1 = TaskManager::instance();
    auto* instance2 = TaskManager::instance();
    QCOMPARE(instance1, instance2);
}

#include "test_task_manager.moc"
