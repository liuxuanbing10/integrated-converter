#include <QTest>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include "../../src/core/config_manager.h"
class TestConfigManager : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        m_testConfigFile = QDir::tempPath() + "/test_config.json";
        QFile::remove(m_testConfigFile);
    }
    void cleanupTestCase() {
        QFile::remove(m_testConfigFile);
    }
    void init() {
        ConfigManager::instance().setValue("maxParallelTasks", 4);
        ConfigManager::instance().setValue("outputDirectory", QDir::tempPath());
        ConfigManager::instance().setValue("logLevel", 1);
    }
    void testDefaultConfig() {
        ConfigManager& config = ConfigManager::instance();
        QVERIFY(config.maxParallelTasks() > 0);
        QVERIFY(!config.outputDirectory().isEmpty());
        QVERIFY(config.logLevel() >= 0);
    }
    void testSetAndGet() {
        ConfigManager& config = ConfigManager::instance();
        config.setValue("testKey", "testValue");
        QCOMPARE(config.value("testKey").toString(), QString("testValue"));
        config.setValue("intKey", 42);
        QCOMPARE(config.value("intKey").toInt(), 42);
        config.setValue("doubleKey", 3.14);
        QCOMPARE(config.value("doubleKey").toDouble(), 3.14);
        config.setValue("boolKey", true);
        QCOMPARE(config.value("boolKey").toBool(), true);
    }
    void testDefaultValue() {
        ConfigManager& config = ConfigManager::instance();
        QCOMPARE(config.value("nonExistentKey", "default").toString(), 
                QString("default"));
        QCOMPARE(config.value("nonExistentInt", 100).toInt(), 100);
    }
    void testSaveAndLoad() {
        ConfigManager& config = ConfigManager::instance();
        config.setValue("maxParallelTasks", 8);
        config.setValue("outputDirectory", "C:/test/output");
        config.setValue("logLevel", 2);
        config.setValue("customKey", "customValue");
        QVERIFY(config.saveConfig(m_testConfigFile));
        ConfigManager& config2 = ConfigManager::instance();
        config2.setValue("maxParallelTasks", 1);
        config2.setValue("outputDirectory", "");
        config2.setValue("logLevel", 0);
        config2.setValue("customKey", "");
        QVERIFY(config2.loadConfig(m_testConfigFile));
        QCOMPARE(config2.maxParallelTasks(), 8);
        QCOMPARE(config2.outputDirectory(), QString("C:/test/output"));
        QCOMPARE(config2.logLevel(), 2);
        QCOMPARE(config2.value("customKey").toString(), QString("customValue"));
    }
    void testMaxParallelTasks() {
        ConfigManager& config = ConfigManager::instance();
        config.setMaxParallelTasks(16);
        QCOMPARE(config.maxParallelTasks(), 16);
        config.setMaxParallelTasks(1);
        QCOMPARE(config.maxParallelTasks(), 1);
        config.setMaxParallelTasks(32);
        QCOMPARE(config.maxParallelTasks(), 32);
    }
    void testOutputDirectory() {
        ConfigManager& config = ConfigManager::instance();
        QString testDir = "C:/custom/output";
        config.setOutputDirectory(testDir);
        QCOMPARE(config.outputDirectory(), testDir);
        testDir = QDir::tempPath();
        config.setOutputDirectory(testDir);
        QCOMPARE(config.outputDirectory(), testDir);
    }
    void testLogLevel() {
        ConfigManager& config = ConfigManager::instance();
        config.setLogLevel(0);
        QCOMPARE(config.logLevel(), 0);
        config.setLogLevel(3);
        QCOMPARE(config.logLevel(), 3);
        config.setLogLevel(1);
        QCOMPARE(config.logLevel(), 1);
    }
    void testConfigChangedSignal() {
        ConfigManager& config = ConfigManager::instance();
        QSignalSpy spy(&config, &ConfigManager::configChanged);
        config.setValue("testSignalKey", "value1");
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), QString("testSignalKey"));
        config.setValue("testSignalKey", "value2");
        QCOMPARE(spy.count(), 2);
    }
    void testInvalidConfigFile() {
        ConfigManager& config = ConfigManager::instance();
        QString invalidFile = "/nonexistent/path/config.json";
        QVERIFY(!config.loadConfig(invalidFile));
    }
    void testAllConfig() {
        ConfigManager& config = ConfigManager::instance();
        config.setValue("key1", "value1");
        config.setValue("key2", 42);
        QVariantMap allConfig = config.allConfig();
        QVERIFY(allConfig.contains("key1"));
        QVERIFY(allConfig.contains("key2"));
        QCOMPARE(allConfig["key1"].toString(), QString("value1"));
        QCOMPARE(allConfig["key2"].toInt(), 42);
    }
    void testSingleton() {
        ConfigManager& instance1 = ConfigManager::instance();
        ConfigManager& instance2 = ConfigManager::instance();
        QCOMPARE(&instance1, &instance2);
    }
private:
    QString m_testConfigFile;
};
#include "test_config_manager.moc"
