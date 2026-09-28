#include "test_main_window.h"

#include "core/config_manager.h"
#include "core/task_manager.h"
#include "ui/main_window.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QPixmap>
#include <QTest>

void TestMainWindow::initTestCase()
{
    // Earlier suites leave dispatched tasks in the singleton TaskManager;
    // their queued onTaskFinished events replay inside qWait's loop AFTER
    // MainWindow wires itself up — MainWindow then pops the modal summary
    // dialog and the run hangs. Drain the queue BEFORE any MainWindow
    // exists (signals with no connected receiver are dropped), and keep the
    // notification off as belt-and-braces.
    ConfigManager::instance().setValue(QStringLiteral("showNotification"), false);
    TaskManager::instance()->cancelAllTasks();
    TaskManager::instance()->waitForTasks(5000);
    for (int i = 0; i < 200 && TaskManager::instance()->totalTaskCount() > 0; ++i)
        QCoreApplication::processEvents();
}

void TestMainWindow::testWindowTitleUsesSingleVersionSource()
{
    // 零.5 guard: the title must render the CMake-injected APP_VERSION —
    // no hardcoded version string may reappear in the UI.
    MainWindow w;
    QCOMPARE(w.windowTitle(), QStringLiteral("集成格式转换工具 v%1").arg(QStringLiteral(APP_VERSION)));
}

void TestMainWindow::testConstructAndQtNativeGrab()
{
    MainWindow w;
    w.resize(1200, 800);
    w.show();
    QTest::qWait(400); // let the first paint + Fusion polish settle
    const QPixmap grab = w.grab();
    QVERIFY(!grab.isNull());
    // DPI scaling: grab() returns DEVICE pixels (1200 logical * 1.25 = 1500
    // on this machine). Pin against the widget's own device geometry, not a
    // hardcoded logical size (strategy §3.2: assert semantics, not environment).
    QCOMPARE(grab.size(), w.size() * w.devicePixelRatio());
    const QString path = QDir::tempPath() + QStringLiteral("/main_window_grab.png");
    QVERIFY(grab.save(path, "PNG"));
    QVERIFY(QFile::exists(path));
    w.close();
}
