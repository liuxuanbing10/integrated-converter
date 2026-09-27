#ifndef TEST_PARAMS_WIDGETS_H
#define TEST_PARAMS_WIDGETS_H

#include <QObject>

/// Smoke + contract tests for the AbstractParamsWidget refactor (§6.5):
/// construction, paramsChanged wiring, and — most importantly — the
/// getParams/setParams round-trip that the dialog's save/load paths depend
/// on. If the base class loses a registered control or the key mapping
/// drifts, these fail loudly.
class TestParamsWidgets : public QObject
{
    Q_OBJECT
public:
    explicit TestParamsWidgets(QObject* parent = nullptr) : QObject(parent)
    { }
private slots:
    void testAudioRoundTrip();
    void testVideoRoundTrip();
    void testImageRoundTrip();
    void testDocumentRoundTrip();
    void testSetParamsDoesNotEmit();
    void testValidateAfterDefaults();
    void testComboInvalidKeyIgnored();
};

#endif // TEST_PARAMS_WIDGETS_H
