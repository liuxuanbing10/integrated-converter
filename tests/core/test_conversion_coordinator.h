#ifndef TEST_CONVERSION_COORDINATOR_H
#define TEST_CONVERSION_COORDINATOR_H

#include <QObject>

/// §零.2 acceptance: the orchestration that left MainWindow must carry its
/// own tests — submit/route/dedup/pause semantics asserted against TaskManager.
class TestConversionCoordinator : public QObject
{
    Q_OBJECT
public:
    explicit TestConversionCoordinator(QObject* parent = nullptr) : QObject(parent)
    { }

private slots:
    void initTestCase();
    void cleanup();
    void testSubmitRoutesAndCounts();
    void testSubmitRetryOnlyPaths();
    void testSubmitDefaultDirIsHome();
    void testLedgerDropFailedKeepsSuccess();
    void testTogglePauseReflectsTaskManager();
};

#endif // TEST_CONVERSION_COORDINATOR_H
