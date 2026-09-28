#ifndef TEST_MAIN_WINDOW_H
#define TEST_MAIN_WINDOW_H

#include <QObject>

/// §零.2 acceptance harness: MainWindow must remain constructible, its
/// chrome must stay intact across the ConversionCoordinator extraction, and
/// a Qt-native grab (the取证 method mandated by §3.2 — no OS screenshot
/// DPI artifacts) must produce a deterministic pixmap. The PNG saved by this
/// suite is byte-compared pre/post refactor.
class TestMainWindow : public QObject
{
    Q_OBJECT
public:
    explicit TestMainWindow(QObject* parent = nullptr) : QObject(parent)
    { }

private slots:
    void initTestCase();
    void testWindowTitleUsesSingleVersionSource();
    void testConstructAndQtNativeGrab();
};

#endif // TEST_MAIN_WINDOW_H
