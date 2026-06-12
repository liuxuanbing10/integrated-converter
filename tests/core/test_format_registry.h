#ifndef TEST_FORMAT_REGISTRY_H
#define TEST_FORMAT_REGISTRY_H

#include <QObject>
#include <QString>

class TestFormatRegistry : public QObject {
    Q_OBJECT
public:
    explicit TestFormatRegistry(QObject* parent = nullptr) : QObject(parent) {}
private slots:
    void testSingletonInstance();
    void testIsVideo();
    void testIsAudio();
    void testIsImage();
    void testIsDocument();
    void testIsSupported();
    void testCategory();
    void testConverterForExt();
    void testVideoFormats();
    void testAudioFormats();
    void testAllFormats();
    void testFFmpegFormatName();
    void testFFmpegVideoCodec();
    void testFFmpegAudioCodec();
    void testPandocFormatName();
    void testImagemagickFormatName();
    void testFileDialogFilter();
};

#endif // TEST_FORMAT_REGISTRY_H
