#ifndef TEST_SEGMENTED_CONVERTER_H
#define TEST_SEGMENTED_CONVERTER_H

#include <QObject>
#include <QString>

class TestSegmentedConverter : public QObject {
    Q_OBJECT
public:
    explicit TestSegmentedConverter(QObject* parent = nullptr) : QObject(parent) {}
private slots:
    void initTestCase();
    void testDefaultConfig();
    void testSetSegmentCount();
    void testSetSegmentCountBoundary();
    void testSetMaxSegments();
    void testSetMaxSegmentsBoundary();
    void testOverallProgressWhenNotRunning();
    void testIsRunningWhenNotRunning();
    void testCurrentSegmentWhenNotRunning();
    void testCancelWhenNotRunning();
    void testProgressChangedSignal();
    void testStatusChangedSignal();
    void testSegmentProgressSignal();
    void testConversionFinishedSignal();
    void testErrorOccurredSignal();
    void testConvertWithoutFFmpeg();
    void testSetFFmpegPath();
    void testSetFFprobePath();
private:
    bool checkFFmpegAvailable();
    bool m_ffmpegAvailable;
};

#endif
