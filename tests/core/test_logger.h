#ifndef TEST_LOGGER_H
#define TEST_LOGGER_H

#include <QObject>
#include <QString>

class TestLogger : public QObject {
    Q_OBJECT
public:
    explicit TestLogger(QObject* parent = nullptr) : QObject(parent) {}
private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void testLogLevel();
    void testLogLevelFiltering();
    void testFileOutput();
    void testMaxFileSize();
    void testModuleFilter();
    void testThreadSafety();
    void testConsoleOutput();
    void testSingleton();
private:
    QString m_testLogFile;
};

#endif
