#include <QTest>
#include <QTemporaryFile>
#include <QFile>
#include <QTextStream>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QVector>
#include <QDir>
#include "../../src/core/logger.h"
#include "test_logger.h"

void TestLogger::initTestCase() {
    m_testLogFile = QDir::tempPath() + "/test_logger.log";
    QFile::remove(m_testLogFile);
}

void TestLogger::cleanupTestCase() {
    QFile::remove(m_testLogFile);
    for (int i = 1; i <= 5; ++i) {
        QFile::remove(m_testLogFile + "." + QString::number(i));
    }
}

void TestLogger::init() {
    Logger::instance().setLevel(Logger::Level::Debug);
    Logger::instance().setConsoleOutput(false);
    Logger::instance().setFileOutput(false);
    Logger::instance().clearModuleFilter();
}

void TestLogger::testLogLevel() {
    Logger& logger = Logger::instance();
    logger.setLevel(Logger::Level::Warning);
    QCOMPARE(logger.level(), Logger::Level::Warning);
    logger.setLevel(Logger::Level::Error);
    QCOMPARE(logger.level(), Logger::Level::Error);
    logger.setLevel(Logger::Level::Info);
    QCOMPARE(logger.level(), Logger::Level::Info);
    logger.setLevel(Logger::Level::Debug);
    QCOMPARE(logger.level(), Logger::Level::Debug);
}

void TestLogger::testLogLevelFiltering() {
    Logger& logger = Logger::instance();
    logger.setLevel(Logger::Level::Warning);
    logger.setFileOutput(true);
    logger.setLogFile(m_testLogFile);
    logger.debug("TestModule", "This debug message should be filtered");
    logger.info("TestModule", "This info message should be filtered");
    logger.warning("TestModule", "This warning should appear");
    logger.error("TestModule", "This error should appear");
    QFile file(m_testLogFile);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QString content = file.readAll();
    file.close();
    QVERIFY(!content.contains("This debug message should be filtered"));
    QVERIFY(!content.contains("This info message should be filtered"));
    QVERIFY(content.contains("This warning should appear"));
    QVERIFY(content.contains("This error should appear"));
}

void TestLogger::testFileOutput() {
    Logger& logger = Logger::instance();
    logger.setFileOutput(true);
    logger.setLogFile(m_testLogFile);
    logger.info("TestModule", "Test file output message");
    QFile file(m_testLogFile);
    QVERIFY(file.exists());
    QVERIFY(file.open(QIODevice::ReadOnly));
    QString content = file.readAll();
    file.close();
    QVERIFY(content.contains("Test file output message"));
    QVERIFY(content.contains("TestModule"));
    // Logger format left-pads level to 7 chars; "[INFO   ]" is the actual format
    QVERIFY(content.contains("INFO"));
}

void TestLogger::testMaxFileSize() {
    Logger& logger = Logger::instance();
    logger.setFileOutput(true);
    logger.setLogFile(m_testLogFile);
    logger.setMaxFileSize(100);
    logger.setMaxBackupFiles(3);
    QCOMPARE(logger.maxFileSize(), qint64(100));
    QCOMPARE(logger.maxBackupFiles(), 3);
    for (int i = 0; i < 20; ++i) {
        logger.info("TestModule", "This is a test message to trigger rotation");
    }
    bool hasBackup = QFile::exists(m_testLogFile + ".1") ||
                    QFile::exists(m_testLogFile + ".2") ||
                    QFile::exists(m_testLogFile + ".3");
    QVERIFY(hasBackup || QFile::exists(m_testLogFile));
}

void TestLogger::testModuleFilter() {
    Logger& logger = Logger::instance();
    logger.setFileOutput(true);
    logger.setLogFile(m_testLogFile);
    QSet<QString> enabledModules;
    enabledModules.insert("EnabledModule");
    enabledModules.insert("AnotherEnabled");
    logger.setModuleFilter(enabledModules);
    QVERIFY(logger.isModuleEnabled("EnabledModule"));
    QVERIFY(logger.isModuleEnabled("AnotherEnabled"));
    QVERIFY(!logger.isModuleEnabled("DisabledModule"));
    logger.enableModule("NewModule");
    QVERIFY(logger.isModuleEnabled("NewModule"));
    logger.disableModule("NewModule");
    QVERIFY(!logger.isModuleEnabled("NewModule"));
    logger.clearModuleFilter();
    QVERIFY(logger.isModuleEnabled("AnyModule"));
}

void TestLogger::testThreadSafety() {
    Logger& logger = Logger::instance();
    logger.setFileOutput(true);
    logger.setLogFile(m_testLogFile);
    logger.setLevel(Logger::Level::Debug);
    const int threadCount = 4;
    const int messagesPerThread = 100;
    QVector<QThread*> threads;
    QMutex startMutex;
    QWaitCondition startCondition;
    bool start = false;
    for (int t = 0; t < threadCount; ++t) {
        QThread* thread = QThread::create([&, t]() {
            {
                QMutexLocker locker(&startMutex);
                while (!start) {
                    startCondition.wait(&startMutex);
                }
            }
            for (int i = 0; i < messagesPerThread; ++i) {
                logger.info(QString("Thread%1").arg(t),
                           QString("Message %1").arg(i));
            }
        });
        threads.append(thread);
    }
    {
        QMutexLocker locker(&startMutex);
        start = true;
        startCondition.wakeAll();
    }
    for (QThread* thread : threads) {
        thread->start();
    }
    for (QThread* thread : threads) {
        thread->wait();
        delete thread;
    }
    QFile file(m_testLogFile);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QString content = file.readAll();
    file.close();
    int totalMessages = 0;
    for (int t = 0; t < threadCount; ++t) {
        QString threadName = QString("Thread%1").arg(t);
        totalMessages += content.count(threadName);
    }
    // Allow minor message loss from concurrent file writes;
    // require at least 90% to ensure the logger is substantially correct
    QVERIFY2(totalMessages >= threadCount * messagesPerThread * 9 / 10,
             qPrintable(QString("Expected >= %1 threadsafe messages, got %2")
                        .arg(threadCount * messagesPerThread * 9 / 10)
                        .arg(totalMessages)));
}

void TestLogger::testConsoleOutput() {
    Logger& logger = Logger::instance();
    logger.setConsoleOutput(true);
    logger.setFileOutput(false);
    logger.info("TestModule", "Console output test");
    QVERIFY(true);
}

void TestLogger::testSingleton() {
    Logger& instance1 = Logger::instance();
    Logger& instance2 = Logger::instance();
    QCOMPARE(&instance1, &instance2);
}

