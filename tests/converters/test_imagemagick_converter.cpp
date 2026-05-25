#include <QTest>
#include <QSignalSpy>
#include <QProcess>
#include <QFile>
#include <QTemporaryFile>
#include "../../src/converters/imagemagick_converter.h"
#include "../../src/core/format_registry.h"
#include "test_imagemagick_converter.h"

bool TestImageMagickConverter::checkImageMagickAvailable() {
    QProcess process;
    process.start("magick", QStringList() << "-version");
    bool available = process.waitForStarted() && process.waitForFinished(3000);
    return available;
}

void TestImageMagickConverter::initTestCase() {
    m_magickAvailable = checkImageMagickAvailable();
}

void TestImageMagickConverter::testSupportedInputFormats() {
    ImageMagickConverter converter;
    QStringList inputFormats = converter.supportedInputFormats();
    QVERIFY(!inputFormats.isEmpty());
    QVERIFY(inputFormats.contains("png") || inputFormats.contains("jpg"));
}

void TestImageMagickConverter::testSupportedOutputFormats() {
    ImageMagickConverter converter;
    QStringList outputFormats = converter.supportedOutputFormats();
    QVERIFY(!outputFormats.isEmpty());
    QVERIFY(outputFormats.contains("png") || outputFormats.contains("jpg"));
}

void TestImageMagickConverter::testName() {
    ImageMagickConverter converter;
    QCOMPARE(converter.name(), QString("ImageMagick"));
}

void TestImageMagickConverter::testIsConversionSupported() {
    ImageMagickConverter converter;
    // Common image-to-image conversions should be supported
    QVERIFY(converter.isConversionSupported("png", "jpg"));
    QVERIFY(converter.isConversionSupported("gif", "png"));
    QVERIFY(converter.isConversionSupported("bmp", "tiff"));
    QVERIFY(converter.isConversionSupported("webp", "ico"));
    // Unsupported input/output should fail
    QVERIFY(!converter.isConversionSupported("xxx", "png"));
    QVERIFY(!converter.isConversionSupported("png", "xxx"));
    QVERIFY(!converter.isConversionSupported("", "png"));
}

void TestImageMagickConverter::testSetMagickPath() {
    ImageMagickConverter converter;
    QString testPath = "C:/custom/path/to/magick.exe";
    converter.setMagickPath(testPath);
    QCOMPARE(converter.magickPath(), testPath);
}

void TestImageMagickConverter::testFormatRegistry() {
    const auto& reg = FormatRegistry::instance();
    QVERIFY(!reg.imageInputFormats().isEmpty());
    QVERIFY(!reg.imageOutputFormats().isEmpty());
    QVERIFY(reg.isImage("png"));
    QVERIFY(reg.isImage("jpg"));
    QVERIFY(!reg.imagemagickFormatName("png").isEmpty());
    QVERIFY(!reg.imagemagickFormatName("jpg").isEmpty());
}

void TestImageMagickConverter::testIsRunning() {
    ImageMagickConverter converter;
    QVERIFY(!converter.isRunning());
}

void TestImageMagickConverter::testProgressChangedSignal() {
    ImageMagickConverter converter;
    QSignalSpy spy(&converter, &ImageMagickConverter::progressChanged);
    QVERIFY(spy.isValid());
}

void TestImageMagickConverter::testStatusChangedSignal() {
    ImageMagickConverter converter;
    QSignalSpy spy(&converter, &ImageMagickConverter::statusChanged);
    QVERIFY(spy.isValid());
}

void TestImageMagickConverter::testConversionFinishedSignal() {
    ImageMagickConverter converter;
    QSignalSpy spy(&converter, &ImageMagickConverter::conversionFinished);
    QVERIFY(spy.isValid());
}

void TestImageMagickConverter::testErrorOccurredSignal() {
    ImageMagickConverter converter;
    QSignalSpy spy(&converter, &ImageMagickConverter::errorOccurred);
    QVERIFY(spy.isValid());
}

void TestImageMagickConverter::testConvertWithoutImageMagick() {
    if (m_magickAvailable) {
        QSKIP("ImageMagick is available, skipping this test");
    }
    ImageMagickConverter converter;
    converter.setMagickPath("/nonexistent/magick");
    bool result = converter.convert("input.png", "output.jpg", QVariantMap());
    QVERIFY(!result);
}

void TestImageMagickConverter::testConvertWithInvalidInput() {
    ImageMagickConverter converter;
    bool result = converter.convert("/nonexistent/input.png", "output.jpg", QVariantMap());
    QVERIFY(!result);
}

void TestImageMagickConverter::testCancel() {
    ImageMagickConverter converter;
    converter.cancel();
    QVERIFY(!converter.isRunning());
}
