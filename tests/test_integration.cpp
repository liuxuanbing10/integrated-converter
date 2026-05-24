#include <QTest>
#include <QFile>
#include <QDir>
#include <QThread>
#include <QEventLoop>
#include "../src/core/task_manager.h"
#include "../src/core/config_manager.h"
#include "../src/core/logger.h"
#include "../src/core/error_handler.h"
#include "../src/core/conversion_task.h"
#include "../src/converters/ffmpeg_converter.h"
#include "../src/converters/pandoc_converter.h"
class TestIntegration : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        m_tempDir = QDir::tempPath() + "/integrated_converter_test";
        QDir().mkpath(m_tempDir);
        Logger::instance().setConsoleOutput(false);
        Logger::instance().setFileOutput(false);
        ConfigManager::instance().setMaxParallelTasks(2);
        ConfigManager::instance().setOutputDirectory(m_tempDir);
        TaskManager::instance()->cancelAllTasks();
        ErrorHandler::instance()->clearErrors();
        ErrorHandler::instance()->setShowDialogs(false);
    }
    void cleanupTestCase() {
        TaskManager::instance()->cancelAllTasks();
        ErrorHandler::instance()->clearErrors();
        QDir(m_tempDir).removeRecursively();
    }
    void init() {
        TaskManager::instance()->cancelAllTasks();
        ErrorHandler::instance()->clearErrors();
    }
    void testFullConversionWorkflow() {
        TaskManager* tm = TaskManager::instance();
        QString inputFile = createTestFile("test_input.txt", "Test content");
        QString outputFile = m_tempDir + "/test_output.txt";
        QString taskId = tm->addTask(inputFile, outputFile, QVariantMap());
        QVERIFY(!taskId.isEmpty());
        ConversionTask* task = tm->getTask(taskId);
        QVERIFY(task != nullptr);
        QCOMPARE(task->status(), ConversionTask::Status::Pending);
        tm->removeTask(taskId);
    }
    void testBatchConversion() {
        TaskManager* tm = TaskManager::instance();
        QStringList taskIds;
        const int taskCount = 5;
        for (int i = 0; i < taskCount; ++i) {
            QString inputFile = createTestFile(
                QString("batch_input_%1.txt").arg(i),
                QString("Content %1").arg(i)
            );
            QString outputFile = m_tempDir + QString("/batch_output_%1.txt").arg(i);
            QString taskId = tm->addTask(inputFile, outputFile, QVariantMap());
            taskIds.append(taskId);
        }
        QCOMPARE(tm->totalTaskCount(), taskCount);
        QCOMPARE(tm->pendingCount(), taskCount);
        for (const QString& taskId : taskIds) {
            tm->removeTask(taskId);
        }
        QCOMPARE(tm->totalTaskCount(), 0);
    }
    void testParallelConversion() {
        TaskManager* tm = TaskManager::instance();
        tm->setMaxParallelTasks(4);
        QStringList taskIds;
        const int taskCount = 8;
        for (int i = 0; i < taskCount; ++i) {
            QString inputFile = createTestFile(
                QString("parallel_input_%1.txt").arg(i),
                QString("Content %1").arg(i)
            );
            QString outputFile = m_tempDir + QString("/parallel_output_%1.txt").arg(i);
            QString taskId = tm->addTask(inputFile, outputFile, QVariantMap());
            taskIds.append(taskId);
        }
        QCOMPARE(tm->totalTaskCount(), taskCount);
        tm->cancelAllTasks();
    }
    void testErrorRecovery() {
        ErrorHandler* eh = ErrorHandler::instance();
        ErrorInfo error(ErrorCode::FileNotFound, "Test file not found");
        error.recoverable = true;
        eh->handleError(error);
        QVERIFY(eh->hasErrors());
        QCOMPARE(eh->errorCount(), 1);
        eh->clearErrors();
        QVERIFY(!eh->hasErrors());
    }
    void testTaskCancellation() {
        TaskManager* tm = TaskManager::instance();
        QString inputFile = createTestFile("cancel_input.txt", "Content");
        QString outputFile = m_tempDir + "/cancel_output.txt";
        QString taskId = tm->addTask(inputFile, outputFile, QVariantMap());
        ConversionTask* task = tm->getTask(taskId);
        QVERIFY(task != nullptr);
        tm->cancelTask(taskId);
        QCOMPARE(task->status(), ConversionTask::Status::Cancelled);
        QVERIFY(task->isCancelled());
        tm->removeTask(taskId);
    }
    void testConfigIntegration() {
        ConfigManager& config = ConfigManager::instance();
        config.setMaxParallelTasks(8);
        config.setOutputDirectory(m_tempDir);
        config.setLogLevel(2);
        TaskManager* tm = TaskManager::instance();
        QCOMPARE(tm->maxParallelTasks(), 8);
    }
    void testLoggerIntegration() {
        Logger& logger = Logger::instance();
        QString logFile = m_tempDir + "/integration.log";
        logger.setLogFile(logFile);
        logger.setFileOutput(true);
        logger.setLevel(Logger::Level::Debug);
        logger.info("IntegrationTest", "Test integration message");
        QFile file(logFile);
        QVERIFY(file.exists());
    }
    void testConverterRegistration() {
        TaskManager* tm = TaskManager::instance();
        auto ffmpegConverter = std::make_shared<FFmpegConverter>();
        auto pandocConverter = std::make_shared<PandocConverter>();
        tm->registerConverter("FFmpeg", ffmpegConverter);
        tm->registerConverter("Pandoc", pandocConverter);
        QStringList converters = tm->availableConverters();
        QVERIFY(converters.contains("FFmpeg"));
        QVERIFY(converters.contains("Pandoc"));
        tm->unregisterConverter("FFmpeg");
        tm->unregisterConverter("Pandoc");
    }
    void testPriorityScheduling() {
        TaskManager* tm = TaskManager::instance();
        QString inputFile1 = createTestFile("priority_low.txt", "Low priority");
        QString inputFile2 = createTestFile("priority_high.txt", "High priority");
        QString inputFile3 = createTestFile("priority_normal.txt", "Normal priority");
        QString taskId1 = tm->addTask(inputFile1, m_tempDir + "/out1.txt", QVariantMap());
        QString taskId2 = tm->addTask(inputFile2, m_tempDir + "/out2.txt", QVariantMap());
        QString taskId3 = tm->addTask(inputFile3, m_tempDir + "/out3.txt", QVariantMap());
        ConversionTask* task1 = tm->getTask(taskId1);
        ConversionTask* task2 = tm->getTask(taskId2);
        ConversionTask* task3 = tm->getTask(taskId3);
        task1->setPriority(ConversionTask::Priority::Low);
        task2->setPriority(ConversionTask::Priority::High);
        task3->setPriority(ConversionTask::Priority::Normal);
        QCOMPARE(task1->priority(), ConversionTask::Priority::Low);
        QCOMPARE(task2->priority(), ConversionTask::Priority::High);
        QCOMPARE(task3->priority(), ConversionTask::Priority::Normal);
        tm->cancelAllTasks();
    }
    void testPauseResume() {
        TaskManager* tm = TaskManager::instance();
        for (int i = 0; i < 3; ++i) {
            QString inputFile = createTestFile(
                QString("pause_input_%1.txt").arg(i),
                QString("Content %1").arg(i)
            );
            tm->addTask(inputFile, m_tempDir + QString("/pause_out_%1.txt").arg(i), QVariantMap());
        }
        tm->pause();
        QVERIFY(tm->isPaused());
        tm->resume();
        QVERIFY(!tm->isPaused());
        tm->cancelAllTasks();
    }
private:
    QString createTestFile(const QString& name, const QString& content) {
        QString filePath = m_tempDir + "/" + name;
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(content.toUtf8());
            file.close();
        }
        return filePath;
    }
    QString m_tempDir;
};

#include "test_integration.moc"



