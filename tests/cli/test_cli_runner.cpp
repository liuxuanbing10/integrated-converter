#include "test_cli_runner.h"

#include "../../src/cli/cli_runner.h"

#include <QHash>
#include <QTemporaryDir>
#include <QTest>

namespace
{
/// Records convert() calls instead of touching real toolchains; run() must
/// route by extension and translate the result into exit codes regardless of
/// what binary actually sits behind the name.
class FakeConverter : public IConverter
{
public:
    explicit FakeConverter(QString label) : m_label(std::move(label))
    { }
    std::unique_ptr<IConverter> clone() const override
    {
        return std::make_unique<FakeConverter>(m_label);
    }
    std::optional<ErrorInfo> convert(const QString& input, const QString& output, const QVariantMap&) override
    {
        calls.append(qMakePair(input, output));
        if (fails)
            return ErrorInfo(ErrorCode::ConversionFailed, QStringLiteral("boom"));
        return std::nullopt;
    }
    QStringList supportedInputFormats() const override
    {
        return {};
    }
    QStringList supportedOutputFormats() const override
    {
        return {};
    }
    QString name() const override
    {
        return m_label;
    }
    bool isConversionSupported(const QString&, const QString&) const override
    {
        return true;
    }
    QList<QPair<QString, QString>> calls;
    bool fails = false;

private:
    QString m_label;
};
} // namespace

void TestCliRunner::testHelpFlag()
{
    QString err;
    auto o = CliRunner::parseArgs({"--help"}, &err);
    QVERIFY(o.showHelp);
    QVERIFY(err.isEmpty());
    o = CliRunner::parseArgs({"-h"}, &err);
    QVERIFY(o.showHelp);
    QVERIFY(err.isEmpty());
    // --help short-circuits validation: no inputs must NOT be an error here.
    o = CliRunner::parseArgs({"--input", "a.mp4", "--help"}, &err);
    QVERIFY(o.showHelp);
    QVERIFY(err.isEmpty());
}

void TestCliRunner::testListFormatsFlag()
{
    QString err;
    auto o = CliRunner::parseArgs({"--list-formats"}, &err);
    QVERIFY(o.listFormats);
    QVERIFY(err.isEmpty());
}

void TestCliRunner::testVerboseFlag()
{
    // Verbose alone still trips the no-input validation (old parser: only
    // --help/--list-formats short-circuit). The pinned contract is that the
    // flag is recognized, NOT that a bare -v is a complete command line.
    QString err;
    auto o = CliRunner::parseArgs({"--verbose"}, &err);
    QVERIFY(o.verbose);
    o = CliRunner::parseArgs({"-v"}, &err);
    QVERIFY(o.verbose);
    err.clear();
    o = CliRunner::parseArgs({"-v", "-i", "a.mp4", "-o", "b.mp3"}, &err);
    QVERIFY(o.verbose);
    QVERIFY(err.isEmpty());
}

void TestCliRunner::testVersionFlag()
{
    // Audit A-1 acceptance: --version is auto-provided (new in the QCommandLineParser
    // migration; the hand-rolled parser rejected it as an unknown option).
    QString err;
    auto o = CliRunner::parseArgs({"--version"}, &err);
    QVERIFY(o.showVersion);
    QVERIFY(err.isEmpty());
}

void TestCliRunner::testInputOutputPairing()
{
    QString err;
    auto o = CliRunner::parseArgs({"-i", "a.mp4", "-o", "a.mp3", "--input", "b.mp4", "--output", "b.mp3"}, &err);
    QVERIFY(err.isEmpty());
    QCOMPARE(o.inputs, QStringList({"a.mp4", "b.mp4"}));
    QCOMPARE(o.outputs, QStringList({"a.mp3", "b.mp3"}));
}

void TestCliRunner::testPositionalInput()
{
    // Positional-only inputs are captured; the missing-destination usage
    // error is EXPECTED (old parser had the same contract: inputs parsed,
    // then validation failed).
    QString err;
    auto o = CliRunner::parseArgs({"x.png", "y.png"}, &err);
    QCOMPARE(o.inputs, QStringList({"x.png", "y.png"}));
    QVERIFY(err.contains(QStringLiteral("--output")));
}

void TestCliRunner::testRepeatedInputKeepsOrder()
{
    QString err;
    auto o = CliRunner::parseArgs({"-i", "a", "-o", "x", "-i", "c", "-o", "y"}, &err);
    QVERIFY(err.isEmpty());
    QCOMPARE(o.inputs, QStringList({"a", "c"}));
    QCOMPARE(o.outputs, QStringList({"x", "y"}));
}

void TestCliRunner::testMixedInputFailsLoud()
{
    // QCommandLineParser groups option values apart from positionals, so the
    // old "-i a b -i c" traversal order cannot be reconstructed. Pairing
    // ambiguity is a data-corruption risk (wrong -o to wrong input), so the
    // migration contract is a LOUD error, not a silent regroup.
    QString err;
    CliRunner::parseArgs({"-i", "a", "b", "-i", "c", "-o", "x", "-o", "y"}, &err);
    QVERIFY(err.contains(QStringLiteral("Cannot mix")));
}

void TestCliRunner::testFormatNormalization()
{
    QString err;
    auto o = CliRunner::parseArgs({"-i", "a.mp4", "--output-dir", "out", "--format", ".MP4"}, &err);
    QVERIFY(err.isEmpty());
    QCOMPARE(o.format, QString("mp4"));
    QCOMPARE(o.outputDir, QString("out"));
}

void TestCliRunner::testConversionParamsMapping()
{
    QString err;
    auto o =
        CliRunner::parseArgs({"-i", "a.mov", "-o", "b.mp4", "--codec", "libx265", "--audio-codec", "aac", "--crf", "28",
                              "--bitrate", "2000", "--audio-bitrate", "192", "--preset", "slow", "-s", "1920x1080"},
                             &err);
    QVERIFY(err.isEmpty());
    QCOMPARE(o.conversionParams.value("videoCodec").toString(), QString("libx265"));
    QCOMPARE(o.conversionParams.value("audioCodec").toString(), QString("aac"));
    QCOMPARE(o.conversionParams.value("crf").toInt(), 28);
    QCOMPARE(o.conversionParams.value("videoBitrate").toInt(), 2000);
    QCOMPARE(o.conversionParams.value("audioBitrate").toInt(), 192);
    QCOMPARE(o.conversionParams.value("preset").toString(), QString("slow"));
    QCOMPARE(o.conversionParams.value("resolution").toString(), QString("1920x1080"));
}

void TestCliRunner::testRepeatedOptionKeepsLast()
{
    QString err;
    auto o = CliRunner::parseArgs({"--crf", "10", "--crf", "30", "-i", "a", "-o", "b"}, &err);
    QVERIFY(err.isEmpty());
    QCOMPARE(o.conversionParams.value("crf").toInt(), 30);
}

void TestCliRunner::testUnknownOptionFails()
{
    QString err;
    CliRunner::parseArgs({"--bogus-flag", "-i", "a"}, &err);
    QVERIFY(!err.isEmpty());
    QVERIFY(err.contains(QStringLiteral("bogus-flag")));
}

void TestCliRunner::testMissingOptionValueFails()
{
    // "--output" at end-of-args: old parser failed with "requires a path".
    // QCommandLineParser fails with its own "did not specify a value" — error
    // wording changes (allowed: the failure itself, exit code 2 at the caller,
    // is the contract that must survive the migration).
    QString err;
    CliRunner::parseArgs({"-i", "a.mp4", "--output"}, &err);
    QVERIFY(!err.isEmpty());
}

void TestCliRunner::testValidationNoInput()
{
    QString err;
    CliRunner::parseArgs({"--format", "mp3"}, &err);
    QVERIFY(err.contains(QStringLiteral("No input files")));
}

void TestCliRunner::testValidationOutputCountMismatch()
{
    QString err;
    CliRunner::parseArgs({"-i", "a.mp4", "-i", "b.mp4", "-o", "only.mp3"}, &err);
    QVERIFY(err.contains(QStringLiteral("must match")));
}

void TestCliRunner::testValidationOutputDirRequiresFormat()
{
    QString err;
    CliRunner::parseArgs({"-i", "a.mp4", "--output-dir", "out"}, &err);
    QVERIFY(err.contains(QStringLiteral("--format")));
}

void TestCliRunner::testRunRoutesAndCounts()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    // One real input file so the exists() gate passes.
    QFile in(tmp.filePath("a.mp4"));
    QVERIFY(in.open(QIODevice::WriteOnly));
    in.write("fake");
    in.close();

    FakeConverter ffmpeg("FFmpeg");
    QHash<QString, IConverter*> byName;
    byName.insert("FFmpeg", &ffmpeg);

    CliRunner::Options o;
    o.inputs = {tmp.filePath("a.mp4")};
    o.outputs = {tmp.filePath("a.mp3")};
    QString err;
    int rc = CliRunner::run(o, byName, &err);
    QCOMPARE(rc, 0);
    QCOMPARE(ffmpeg.calls.size(), 1);
    QCOMPARE(ffmpeg.calls.first().first, tmp.filePath("a.mp4"));
    QCOMPARE(ffmpeg.calls.first().second, tmp.filePath("a.mp3"));
}

void TestCliRunner::testRunFailureExitCode()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QFile in(tmp.filePath("a.mp4"));
    QVERIFY(in.open(QIODevice::WriteOnly));
    in.write("fake");
    in.close();

    FakeConverter ffmpeg("FFmpeg");
    ffmpeg.fails = true;
    QHash<QString, IConverter*> byName;
    byName.insert("FFmpeg", &ffmpeg);

    CliRunner::Options o;
    o.inputs = {tmp.filePath("a.mp4")};
    o.outputs = {tmp.filePath("a.mp3")};
    QString err;
    QCOMPARE(CliRunner::run(o, byName, &err), 1);
}

void TestCliRunner::testRunSkipsMissingFile()
{
    FakeConverter ffmpeg("FFmpeg");
    QHash<QString, IConverter*> byName;
    byName.insert("FFmpeg", &ffmpeg);

    CliRunner::Options o;
    o.inputs = {"Z:/definitely/not/here.mp4"};
    o.outputs = {"Z:/out.mp3"};
    QString err;
    int rc = CliRunner::run(o, byName, &err);
    QCOMPARE(rc, 1);
    QCOMPARE(ffmpeg.calls.size(), 0);
}
