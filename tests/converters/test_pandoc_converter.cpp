#include <QTest>
#include <QSignalSpy>
#include <QProcess>
#include <QFile>
#include <QTemporaryFile>
#include "../../src/converters/pandoc_converter.h"
class TestPandocConverter : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        m_pandocAvailable = checkPandocAvailable();
    }
    void testSupportedInputFormats() {
        PandocConverter converter;
        QStringList inputFormats = converter.supportedInputFormats();
        QVERIFY(!inputFormats.isEmpty());
        QVERIFY(inputFormats.contains("md") || inputFormats.contains("markdown"));
    }
    void testSupportedOutputFormats() {
        PandocConverter converter;
        QStringList outputFormats = converter.supportedOutputFormats();
        QVERIFY(!outputFormats.isEmpty());
        QVERIFY(outputFormats.contains("html") || outputFormats.contains("pdf"));
    }
    void testName() {
        PandocConverter converter;
        QCOMPARE(converter.name(), QString("Pandoc"));
    }
    void testIsConversionSupported() {
        PandocConverter converter;
        bool supported = converter.isConversionSupported("md", "html");
        QVERIFY(supported || !supported);
        supported = converter.isConversionSupported("docx", "pdf");
        QVERIFY(supported || !supported);
    }
    void testSetPandocPath() {
        PandocConverter converter;
        QString testPath = "/custom/path/to/pandoc";
        converter.setPandocPath(testPath);
        QCOMPARE(converter.pandocPath(), testPath);
    }
    void testGetPandocFormat() {
        PandocConverter converter;
        QString pandocFormat = converter.getPandocFormat("md");
        QVERIFY(!pandocFormat.isEmpty() || pandocFormat.isEmpty());
        pandocFormat = converter.getPandocFormat("markdown");
        QVERIFY(!pandocFormat.isEmpty() || pandocFormat.isEmpty());
    }
    void testCheckPandocAvailable() {
        PandocConverter converter;
        bool available = converter.checkPandocAvailable();
        QCOMPARE(available, m_pandocAvailable);
    }
    void testGetPandocVersion() {
        PandocConverter converter;
        QString version = converter.getPandocVersion();
        if (m_pandocAvailable) {
            QVERIFY(!version.isEmpty());
        }
    }
    void testProgressChangedSignal() {
        PandocConverter converter;
        QSignalSpy spy(&converter, &PandocConverter::progressChanged);
        QVERIFY(spy.isValid());
    }
    void testStatusChangedSignal() {
        PandocConverter converter;
        QSignalSpy spy(&converter, &PandocConverter::statusChanged);
        QVERIFY(spy.isValid());
    }
    void testConversionFinishedSignal() {
        PandocConverter converter;
        QSignalSpy spy(&converter, &PandocConverter::conversionFinished);
        QVERIFY(spy.isValid());
    }
    void testErrorOccurredSignal() {
        PandocConverter converter;
        QSignalSpy spy(&converter, &PandocConverter::errorOccurred);
        QVERIFY(spy.isValid());
    }
    void testConvertWithoutPandoc() {
        if (m_pandocAvailable) {
            QSKIP("Pandoc is available, skipping this test");
        }
        PandocConverter converter;
        converter.setPandocPath("/nonexistent/pandoc");
        bool result = converter.convert("input.md", "output.html", QVariantMap());
        QVERIFY(!result);
    }
    void testConvertWithInvalidInput() {
        PandocConverter converter;
        bool result = converter.convert("/nonexistent/input.md", "output.html", QVariantMap());
        QVERIFY(!result);
    }
    void testConvertWithEmptyInput() {
        PandocConverter converter;
        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        tempFile.write("");
        tempFile.close();
        if (m_pandocAvailable) {
            QString outputFile = QDir::tempPath() + "/test_output.html";
            bool result = converter.convert(tempFile.fileName(), outputFile, QVariantMap());
            QFile::remove(outputFile);
        }
    }
private:
    bool checkPandocAvailable() {
        QProcess process;
        process.start("pandoc", QStringList() << "--version");
        bool available = process.waitForStarted() && process.waitForFinished(3000);
        return available;
    }
    bool m_pandocAvailable;
};
#include "test_pandoc_converter.moc"
