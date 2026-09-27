#include "test_task_manager.h"

#include "../../src/core/conversion_task.h"
#include "../../src/core/iconverter.h"
#include "../../src/core/task_manager.h"

#include <QEventLoop>
#include <QSignalSpy>
#include <QTest>
#include <QThread>

// MockConverter: test mock, keep inline with Q_OBJECT (no separate header needed)
class MockConverter : public QObject, public IConverter
{
    Q_OBJECT
public:
    explicit MockConverter(QObject* parent = nullptr) : QObject(parent)
    { }
    MockConverter(const MockConverter& other) : QObject(nullptr)
    {
        m_calls = other.m_calls;
        m_delayMs = other.m_delayMs;
        m_reportProgress = other.m_reportProgress;
    }
    std::unique_ptr<IConverter> clone() const override
    {
        return std::make_unique<MockConverter>(*this);
    }
    QString name() const override
    {
        return "MockConverter";
    }
    QStringList supportedInputFormats() const override
    {
        return {"mock_in"};
    }
    QStringList supportedOutputFormats() const override
    {
        return {"mock_out"};
    }
    std::optional<ErrorInfo> convert(const QString& inputFile, const QString& outputFile,
                                     const QVariantMap& params) override
    {
        Q_UNUSED(inputFile);
        Q_UNUSED(outputFile);
        Q_UNUSED(params);
        ++m_calls;
        if (m_delayMs > 0)
        {
            // Run on the WORKER thread (TaskRunnable::run). No QEventLoop
            // here — no Qt event objects are created in this thread, so
            // plain sleeps are safe and keep the process poll-free.
            QThread::msleep(m_delayMs);
        }
        if (m_reportProgress > 0 && m_progressCb)
        {
            m_progressCb(m_reportProgress);
        }
        return std::nullopt;
    }
    int callCount() const
    {
        return m_calls;
    }
    void setDelay(int ms)
    {
        m_delayMs = ms;
    }
    void setReportProgress(int p)
    {
        m_reportProgress = p;
    }
    bool isConversionSupported(const QString& inputFormat, const QString& outputFormat) const override
    {
        return inputFormat == "mock_in" && outputFormat == "mock_out";
    }

private:
    int m_calls = 0;
    int m_delayMs = 0;
    int m_reportProgress = 0;
};

void TestTaskManager::initTestCase()
{
    TaskManager::instance()->cancelAllTasks();
}

void TestTaskManager::cleanupTestCase()
{
    TaskManager::instance()->cancelAllTasks();
}

void TestTaskManager::init()
{
    TaskManager::instance()->cancelAllTasks();
    TaskManager::instance()->registerConverter("mock", std::make_shared<MockConverter>());
}

void TestTaskManager::testRegisterConverter()
{
    auto* tm = TaskManager::instance();
    auto* conv = tm->converter("mock");
    QVERIFY(conv != nullptr);
    QCOMPARE(conv->name(), QString("MockConverter"));
}

void TestTaskManager::testAddTask()
{
    auto* tm = TaskManager::instance();
    int before = tm->totalTaskCount();
    QString taskId = tm->addTask("/input.mock_in", "/output.mock_out", QVariantMap());
    QVERIFY(!taskId.isEmpty());
    QCOMPARE(tm->totalTaskCount(), before + 1);
}

void TestTaskManager::testRemoveTask()
{
    auto* tm = TaskManager::instance();
    int before = tm->totalTaskCount();
    QString taskId = tm->addTask("/input.mock_in", "/output.mock_out", QVariantMap());
    QVERIFY(!taskId.isEmpty());
    tm->removeTask(taskId);
    QCOMPARE(tm->totalTaskCount(), before);
}

void TestTaskManager::testCancelTask()
{
    auto* tm = TaskManager::instance();
    QString taskId = tm->addTask("/input.mock_in", "/output.mock_out", QVariantMap());
    QVERIFY(!taskId.isEmpty());
    tm->cancelTask(taskId);
    auto* task = tm->getTask(taskId);
    QVERIFY(task != nullptr);
    QVERIFY(task->status() == ConversionTask::Status::Cancelled);
}

void TestTaskManager::testCancelAllTasks()
{
    auto* tm = TaskManager::instance();
    tm->cancelAllTasks();
    for (int i = 0; i < 5; ++i)
    {
        tm->addTask("/input.mock_in", "/output.mock_out", QVariantMap());
    }
    tm->cancelAllTasks();
    QCOMPARE(tm->pendingCount(), 0);
    QCOMPARE(tm->runningCount(), 0);
}

void TestTaskManager::testMaxParallelTasks()
{
    auto* tm = TaskManager::instance();
    int original = tm->maxParallelTasks();
    // Do NOT assert an absolute value: the setter clamps to
    // idealThreadCount()*2, so e.g. 10 survives on a 16-core dev box
    // but is truncated to 8 on a 4-core CI runner. Assert the
    // clamping semantics instead.
    tm->setMaxParallelTasks(3);
    QCOMPARE(tm->maxParallelTasks(), 3);
    // 0 and negatives must clamp to 1, not disable the queue.
    tm->setMaxParallelTasks(0);
    QCOMPARE(tm->maxParallelTasks(), 1);
    // Huge values clamp to the cap; reading it back proves the bound.
    tm->setMaxParallelTasks(100000);
    QCOMPARE(tm->maxParallelTasks(), qMax(1, QThread::idealThreadCount() * 2));
    tm->setMaxParallelTasks(original);
    QCOMPARE(tm->maxParallelTasks(), original);
}

void TestTaskManager::testTaskCounts()
{
    auto* tm = TaskManager::instance();
    int before = tm->totalTaskCount();
    for (int i = 0; i < 3; ++i)
    {
        tm->addTask("/input.mock_in", "/output.mock_out", QVariantMap());
    }
    QCOMPARE(tm->totalTaskCount(), before + 3);
}

void TestTaskManager::testGetTasksByStatus()
{
    auto* tm = TaskManager::instance();
    tm->cancelAllTasks();
    tm->addTask("/input.mock_in", "/output.mock_out", QVariantMap());
    QVERIFY(tm->pendingCount() > 0);
}

void TestTaskManager::testStart()
{
    auto* tm = TaskManager::instance();
    int originalMax = tm->maxParallelTasks();
    tm->setMaxParallelTasks(4);
    tm->addTask("/input.mock_in", "/output.mock_out", QVariantMap());
    tm->start();
    QVERIFY(tm->totalTaskCount() > 0);
    tm->cancelAllTasks();
    tm->setMaxParallelTasks(originalMax);
}

void TestTaskManager::testTaskAddedSignal()
{
    auto* tm = TaskManager::instance();
    QSignalSpy spy(tm, &TaskManager::taskAdded);
    tm->addTask("/input.mock_in", "/output.mock_out", QVariantMap());
    QCOMPARE(spy.count(), 1);
}

// ── Regression: shared-singleton converters made batch conversion fail ──
// Before the clone-per-task fix, 4 parallel tasks dispatched onto ONE
// converter instance tripped the "already running" guard and 3 of 4 failed.
void TestTaskManager::testParallelTasksAllSucceed()
{
    auto* tm = TaskManager::instance();
    tm->cancelAllTasks();

    auto mock = std::make_shared<MockConverter>();
    mock->setDelay(300); // hold each "conversion" long enough to overlap
    tm->registerConverter("mock", mock);
    int originalMax = tm->maxParallelTasks();
    tm->setMaxParallelTasks(4);

    // Earlier tests leave (cancelled/failed) tasks in m_tasks — assert on
    // DELTAS, not absolute counters.
    const auto before = tm->counters();

    QSignalSpy doneSpy(tm, &TaskManager::allTasksCompleted);
    QStringList ids;
    QVariantMap params;
    params["converter"] = "mock";
    for (int i = 0; i < 4; ++i)
    {
        ids << tm->addTask(QString("/input%1.mock_in").arg(i), QString("/output%1.mock_out").arg(i), params);
    }
    tm->start();

    // Pump the event loop until all four finish (queued from worker threads).
    QSignalSpy oneDone(tm, &TaskManager::taskCompleted);
    while (oneDone.count() < 4)
    {
        QVERIFY2(oneDone.wait(2000), "parallel task did not complete within timeout");
    }
    QCOMPARE(tm->runningCount(), 0);
    const auto after = tm->counters();
    // The pre-clone bug made 3 of 4 batch tasks fail with "已有转换任务在运行".
    QCOMPARE(after.failed - before.failed, 0);
    QCOMPARE(after.completed - before.completed, 4);
    tm->setMaxParallelTasks(originalMax);
    Q_UNUSED(doneSpy);
}

// ── Pause = no NEW dispatch; resume continues the queue ──
void TestTaskManager::testPauseBlocksDispatchAndResumeContinues()
{
    auto* tm = TaskManager::instance();
    tm->cancelAllTasks();
    // Defensive: a previous run may have left the manager paused.
    if (tm->isPaused())
        tm->resume();

    auto mock = std::make_shared<MockConverter>();
    mock->setDelay(400);
    tm->registerConverter("mock", mock);
    const int originalMax = tm->maxParallelTasks();
    tm->setMaxParallelTasks(1); // serialize: dispatch order is deterministic

    QVariantMap params;
    params["converter"] = "mock";
    const QString id1 = tm->addTask("/in1.mock_in", "/out1.mock_out", params);
    const QString id2 = tm->addTask("/in2.mock_in", "/out2.mock_out", params);

    QSignalSpy startedSpy(tm, &TaskManager::taskStarted);
    QSignalSpy doneSpy(tm, &TaskManager::taskCompleted);
    QSignalSpy pauseSpy(tm, &TaskManager::pauseStateChanged);
    tm->start();

    // First task must get going.
    while (startedSpy.count() < 1)
        QVERIFY2(startedSpy.wait(2000), "first task never started");

    tm->pause();
    QCOMPARE(pauseSpy.count(), 1);
    QVERIFY(tm->isPaused());

    // First task finishes (running work is NOT interrupted by pause).
    while (doneSpy.count() < 1)
        QVERIFY2(doneSpy.wait(2000), "running task interrupted by pause");

    // With the queue paused, the second task must stay Pending.
    QTest::qWait(900);
    QCOMPARE(tm->runningCount(), 0);
    QVERIFY(tm->getTask(id2));
    QCOMPARE((int)tm->getTask(id2)->status(), (int)ConversionTask::Status::Pending);

    tm->resume();
    QCOMPARE(pauseSpy.count(), 2);
    while (doneSpy.count() < 2)
        QVERIFY2(doneSpy.wait(2000), "resumed queue never dispatched task 2");
    QVERIFY(tm->getTask(id1));
    QCOMPARE((int)tm->getTask(id1)->status(), (int)ConversionTask::Status::Completed);

    tm->setMaxParallelTasks(originalMax);
}

// ── Regression: TaskRunnable::progressChanged was never emitted (§3.2) ──
void TestTaskManager::testProgressPropagation()
{
    auto* tm = TaskManager::instance();
    tm->cancelAllTasks();

    auto mock = std::make_shared<MockConverter>();
    mock->setReportProgress(42);
    tm->registerConverter("mock", mock);

    QVariantMap params;
    params["converter"] = "mock";
    // Spies BEFORE addTask: m_started may already be true, so the task can
    // dispatch (and finish) the instant it is added.
    QSignalSpy progressSpy(tm, &TaskManager::taskProgressChanged);
    QSignalSpy doneSpy(tm, &TaskManager::taskCompleted);
    QString taskId = tm->addTask("/input.mock_in", "/output.mock_out", params);
    tm->start();

    // taskCompleted also drives the progress bar to 100; we want the
    // mid-conversion 42 emitted through the converter's progress callback.
    bool saw42 = false;
    for (int spins = 0; spins < 100 && !saw42; ++spins)
    {
        doneSpy.wait(200);
        for (const auto& args : progressSpy)
        {
            if (args.at(0).toString() == taskId && args.at(1).toInt() == 42)
            {
                saw42 = true;
                break;
            }
        }
    }
    QVERIFY(saw42);
    QVERIFY(!doneSpy.isEmpty());
}

// ── Regression: cancel(A) must not kill B's process (§3.1 friendly fire) ──
void TestTaskManager::testCancelIsPerTask()
{
    auto* tm = TaskManager::instance();
    tm->cancelAllTasks();

    auto mock = std::make_shared<MockConverter>();
    mock->setDelay(1500);
    tm->registerConverter("mock", mock);
    int originalMax = tm->maxParallelTasks();
    tm->setMaxParallelTasks(4);

    // m_started may already be true from earlier tests (singleton), so
    // addTask() can dispatch immediately — the spy must exist BEFORE adding.
    QSignalSpy startedSpy(tm, &TaskManager::taskStarted);
    QVariantMap params;
    params["converter"] = "mock";
    QString idA = tm->addTask("/a.mock_in", "/a.mock_out", params);
    QString idB = tm->addTask("/b.mock_in", "/b.mock_out", params);
    tm->start();

    // Let both enter convert(), then cancel only A.
    while (startedSpy.count() < 2)
    {
        QVERIFY(startedSpy.wait(3000));
    }
    tm->cancelTask(idA);

    // Wait for both to finish.
    QSignalSpy doneSpy(tm, &TaskManager::taskCompleted);
    while (doneSpy.count() < 2)
    {
        QVERIFY(doneSpy.wait(5000));
    }

    auto* taskA = tm->getTask(idA);
    auto* taskB = tm->getTask(idB);
    QVERIFY(taskA && taskB);
    // A must be cancelled, B must NOT be collateral damage — it completes.
    QCOMPARE(int(taskA->status()), int(ConversionTask::Status::Cancelled));
    QCOMPARE(int(taskB->status()), int(ConversionTask::Status::Completed));
    tm->setMaxParallelTasks(originalMax);
}

void TestTaskManager::testTaskRemovedSignal()
{
    auto* tm = TaskManager::instance();
    QString taskId = tm->addTask("/input.mock_in", "/output.mock_out", QVariantMap());
    QSignalSpy spy(tm, &TaskManager::taskRemoved);
    tm->removeTask(taskId);
    QCOMPARE(spy.count(), 1);
}

void TestTaskManager::testSingleton()
{
    auto* instance1 = TaskManager::instance();
    auto* instance2 = TaskManager::instance();
    QCOMPARE(instance1, instance2);
}

#include "test_task_manager.moc"
