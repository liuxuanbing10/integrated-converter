#include <QTest>
#include <QFile>
#include <QDir>
#include <QElapsedTimer>
#include <QThread>
#include "../src/core/task_manager.h"
#include "../src/core/config_manager.h"
#include "../src/core/logger.h"
#include "../src/core/conversion_task.h"
class TestPerformance : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        m_tempDir = QDir::tempPath() + "/performance_test";
        QDir().mkpath(m_tempDir);
        Logger::instance().setConsoleOutput(false);
        Logger::instance().setFileOutput(false);
        ConfigManager::instance().setMaxParallelTasks(4);
        TaskManager::instance()->cancelAllTasks();
    }
    void cleanupTestCase() {
        TaskManager::instance()->cancelAllTasks();
        QDir(m_tempDir).removeRecursively();
    }
    void init() {
        TaskManager::instance()->cancelAllTasks();
    }
    void testTaskCreationPerformance() {
        const int taskCount = 1000;
        QElapsedTimer timer;
        timer.start();
        TaskManager* tm = TaskManager::instance();
        for (int i = 0; i < taskCount; ++i) {
            QString inputFile = m_tempDir + QString("/perf_input_%1.txt").arg(i);
            QString outputFile = m_tempDir + QString("/perf_output_%1.txt").arg(i);
            tm->addTask(inputFile, outputFile, QVariantMap());
        }
        qint64 elapsed = timer.elapsed();
        qDebug() << "Task creation time for" << taskCount << "tasks:" << elapsed << "ms";
        QVERIFY(elapsed < 5000);
        tm->cancelAllTasks();
    }
    void testTaskRemovalPerformance() {
        TaskManager* tm = TaskManager::instance();
        const int taskCount = 500;
        QStringList taskIds;
        for (int i = 0; i < taskCount; ++i) {
            QString inputFile = m_tempDir + QString("/remove_input_%1.txt").arg(i);
            QString outputFile = m_tempDir + QString("/remove_output_%1.txt").arg(i);
            taskIds.append(tm->addTask(inputFile, outputFile, QVariantMap()));
        }
        QElapsedTimer timer;
        timer.start();
        for (const QString& taskId : taskIds) {
            tm->removeTask(taskId);
        }
        qint64 elapsed = timer.elapsed();
        qDebug() << "Task removal time for" << taskCount << "tasks:" << elapsed << "ms";
        QVERIFY(elapsed < 3000);
    }
    void testParallelEfficiency() {
        TaskManager* tm = TaskManager::instance();
        const int taskCount = 100;
        for (int parallel : {1, 2, 4, 8}) {
            tm->setMaxParallelTasks(parallel);
            tm->cancelAllTasks();
            for (int i = 0; i < taskCount; ++i) {
                QString inputFile = m_tempDir + QString("/parallel_%1_input_%2.txt").arg(parallel).arg(i);
                QString outputFile = m_tempDir + QString("/parallel_%1_output_%2.txt").arg(parallel).arg(i);
                tm->addTask(inputFile, outputFile, QVariantMap());
            }
            QCOMPARE(tm->totalTaskCount(), taskCount);
            tm->cancelAllTasks();
        }
    }
    void testMemoryUsageWithManyTasks() {
        TaskManager* tm = TaskManager::instance();
        const int taskCount = 10000;
        qDebug() << "Creating" << taskCount << "tasks for memory test...";
        for (int i = 0; i < taskCount; ++i) {
            QString inputFile = m_tempDir + QString("/mem_input_%1.txt").arg(i);
            QString outputFile = m_tempDir + QString("/mem_output_%1.txt").arg(i);
            tm->addTask(inputFile, outputFile, QVariantMap());
        }
        QCOMPARE(tm->totalTaskCount(), taskCount);
        qDebug() << "Tasks created successfully";
        tm->cancelAllTasks();
        QCOMPARE(tm->totalTaskCount(), 0);
    }
    void testConfigAccessPerformance() {
        ConfigManager& config = ConfigManager::instance();
        const int accessCount = 10000;
        config.setValue("perfKey", "perfValue");
        QElapsedTimer timer;
        timer.start();
        for (int i = 0; i < accessCount; ++i) {
            config.value("perfKey");
        }
        qint64 elapsed = timer.elapsed();
        qDebug() << "Config access time for" << accessCount << "reads:" << elapsed << "ms";
        QVERIFY(elapsed < 1000);
    }
    void testLoggerPerformance() {
        Logger& logger = Logger::instance();
        logger.setLevel(Logger::Level::Debug);
        logger.setFileOutput(false);
        logger.setConsoleOutput(false);
        const int logCount = 10000;
        QElapsedTimer timer;
        timer.start();
        for (int i = 0; i < logCount; ++i) {
            logger.debug("PerfTest", QString("Log message %1").arg(i));
        }
        qint64 elapsed = timer.elapsed();
        qDebug() << "Logger time for" << logCount << "messages:" << elapsed << "ms";
        QVERIFY(elapsed < 5000);
    }
    void testLargeFileHandling() {
        QString largeFile = m_tempDir + "/large_test_file.bin";
        const qint64 fileSize = 10 * 1024 * 1024;
        QFile file(largeFile);
        if (file.open(QIODevice::WriteOnly)) {
            QByteArray chunk(1024 * 1024, 'A');
            for (int i = 0; i < 10; ++i) {
                file.write(chunk);
            }
            file.close();
        }
        QFileInfo info(largeFile);
        QVERIFY(info.exists());
        QCOMPARE(info.size(), fileSize);
        QFile::remove(largeFile);
    }
    void testConcurrentTaskOperations() {
        TaskManager* tm = TaskManager::instance();
        const int threadCount = 4;
        const int tasksPerThread = 100;
        QVector<QThread*> threads;
        for (int t = 0; t < threadCount; ++t) {
            QThread* thread = QThread::create([&, t]() {
                for (int i = 0; i < tasksPerThread; ++i) {
                    QString inputFile = m_tempDir + QString("/concurrent_%1_%2_in.txt").arg(t).arg(i);
                    QString outputFile = m_tempDir + QString("/concurrent_%1_%2_out.txt").arg(t).arg(i);
                    tm->addTask(inputFile, outputFile, QVariantMap());
                }
            });
            threads.append(thread);
        }
        QElapsedTimer timer;
        timer.start();
        for (QThread* thread : threads) {
            thread->start();
        }
        for (QThread* thread : threads) {
            thread->wait();
            delete thread;
        }
        qint64 elapsed = timer.elapsed();
        qDebug() << "Concurrent task creation time:" << elapsed << "ms";
        QCOMPARE(tm->totalTaskCount(), threadCount * tasksPerThread);
        tm->cancelAllTasks();
    }
private:
    QString m_tempDir;
};
#include "test_performance.moc"
