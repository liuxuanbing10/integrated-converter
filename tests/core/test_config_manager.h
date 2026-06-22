#ifndef TEST_CONFIG_MANAGER_H
#define TEST_CONFIG_MANAGER_H

#include <QObject>
#include <QString>

class TestConfigManager : public QObject {
    Q_OBJECT
public:
    explicit TestConfigManager(QObject* parent = nullptr) : QObject(parent) {}
private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void testDefaultConfig();
    void testSetAndGet();
    void testDefaultValue();
    void testSaveAndLoad();
    void testMaxParallelTasks();
    void testOutputDirectory();
    void testLogLevel();
    void testInvalidConfigFile();
    void testAllConfig();
    void testSingleton();
private:
    QString m_testConfigFile;
};

#endif
