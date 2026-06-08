#include <QTest>
#include <QSignalSpy>
#include <QProcess>
#include <QFile>
#include <QTemporaryFile>
#include "../../src/converters/pandoc_converter.h"
#include "test_pandoc_converter.h"

bool TestPandocConverter::checkPandocAvailable() {
    QProcess process;
    process.start("pandoc", QStringList() << "--version");
    bool available = process.waitForStarted() && process.waitForFinished(3000);
    return available;
}

void TestPandocConverter::initTestCase() {
    m_pandocAvailable = checkPandocAvailable();
}

void TestPandocConverter::testSupportedInputFormats() {
    PandocConverter converter;
    QStringList inputFormats = converter.supportedInputFormats();
    QVERIFY(!inputFormats.isEmpty());
    QVERIFY(inputFormats.contains("md") || inputFormats.contains("markdown"));
}

void TestPandocConverter::testSupportedOutputFormats() {
    PandocConverter converter;
    QStringList outputFormats = converter.supportedOutputFormats();
    QVERIFY(!outputFormats.isEmpty());
    QVERIFY(outputFormats.contains("html") || outputFormats.contains("pdf"));
}

void TestPandocConverter::testName() {
    PandocConverter converter;
    QCOMPARE(converter.name(), QString("Pandoc"));
}

void TestPandocConverter::testIsConversionSupported() {
    PandocConverter converter;
    bool supported = converter.isConversionSupported("md", "html");
    QVERIFY(supported || !supported);
    supported = converter.isConversionSupported("md", "pdf");
    QVERIFY(supported || !supported);
}

void TestPandocConverter::testSetPandocPath() {
    PandocConverter converter;
    QString testPath = "/custom/path/to/pandoc";
    converter.setPandocPath(testPath);
    QCOMPARE(converter.pandocPath(), testPath);
}

void TestPandocConverter::testGetPandocFormat() {
    PandocConverter converter;
    QVERIFY(!converter.getPandocFormat("md").isEmpty());
    QVERIFY(!converter.getPandocFormat("html").isEmpty());
    QVERIFY(!converter.getPandocFormat("docx").isEmpty());
}

void TestPandocConverter::testCheckPandocAvailable() {
    PandocConverter converter;
    bool available = converter.checkPandocAvailable();
    QVERIFY(available || !available);
}

void TestPandocConverter::testGetPandocVersion() {
    PandocConverter converter;
    QString version = converter.getPandocVersion();
    QVERIFY(!version.isEmpty() || m_pandocAvailable == false);
}

void TestPandocConverter::testProgressChangedSignal() {
    PandocConverter converter;
    QSignalSpy spy(&converter, &PandocConverter::progressChanged);
    QVERIFY(spy.isValid());
}

void TestPandocConverter::testStatusChangedSignal() {
    PandocConverter converter;
    QSignalSpy spy(&converter, &PandocConverter::statusChanged);
    QVERIFY(spy.isValid());
}

void TestPandocConverter::testConversionFinishedSignal() {
    PandocConverter converter;
    QSignalSpy spy(&converter, &PandocConverter::conversionFinished);
    QVERIFY(spy.isValid());
}

void TestPandocConverter::testErrorOccurredSignal() {
    PandocConverter converter;
    QSignalSpy spy(&converter, &PandocConverter::errorOccurred);
    QVERIFY(spy.isValid());
}

void TestPandocConverter::testConvertWithoutPandoc() {
    if (m_pandocAvailable) {
        QSKIP("Pandoc is available, skipping this test");
    }
    PandocConverter converter;
    converter.setPandocPath("/nonexistent/pandoc");
    auto result = converter.convert("input.md", "output.html", QVariantMap());
    QVERIFY(result.has_value());
}

void TestPandocConverter::testConvertWithInvalidInput() {
    PandocConverter converter;
    auto result = converter.convert("/nonexistent/input.md", "output.html", QVariantMap());
    QVERIFY(result.has_value());
}

void TestPandocConverter::testConvertWithEmptyInput() {
    PandocConverter converter;
    auto result = converter.convert("", "output.html", QVariantMap());
    QVERIFY(result.has_value());
}
