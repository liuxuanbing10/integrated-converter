#include "test_conversion_planner.h"

#include "core/conversion_planner.h"

#include <QFileInfo>
#include <QTest>

using ConversionPlanner::converterNameFor;
using ConversionPlanner::mergeParams;
using ConversionPlanner::outputPath;
using ConversionPlanner::parseBitrateToKbps;

void TestConversionPlanner::testParseBitrateToKbps()
{
    QCOMPARE(parseBitrateToKbps("500k"), 500);
    QCOMPARE(parseBitrateToKbps("2M"), 2000);
    QCOMPARE(parseBitrateToKbps("1500"), 1500);
    QCOMPARE(parseBitrateToKbps("auto"), 0);
    QCOMPARE(parseBitrateToKbps(QString()), 0);
    QCOMPARE(parseBitrateToKbps("junk"), 0);
}

void TestConversionPlanner::testOutputPathExplicitWins()
{
    QCOMPARE(outputPath("/in/a.mp3", "/weird/name.wav", "/out", "mp4"), QStringLiteral("/weird/name.wav"));
}

void TestConversionPlanner::testOutputPathDirNaming()
{
    QCOMPARE(outputPath("/in/report.docx", QString(), "/out", "pdf"), QStringLiteral("/out/report.pdf"));
    QCOMPARE(outputPath("/in/report.docx", QString(), "/out", ".pdf"), QStringLiteral("/out/report.pdf"));
    // empty format keeps the source extension
    QCOMPARE(outputPath("/in/photo.jpeg", QString(), "/out", QString()), QStringLiteral("/out/photo.jpeg"));
    // completeBaseName: dots inside the stem survive
    QCOMPARE(outputPath("/in/my.tape.v2.mkv", QString(), "/out", "mp4"), QStringLiteral("/out/my.tape.v2.mp4"));
}

void TestConversionPlanner::testOutputPathNeverOverwritesSource()
{
    // Same dir + same ext as the input -> renamed, never clobbered.
    const QString out = outputPath("out/x.png", QString(), "out", "png");
    QVERIFY(!out.endsWith("out/x.png") && out.endsWith("_converted.png"));
    QVERIFY(QFileInfo(out).absoluteFilePath() != QFileInfo(QStringLiteral("out/x.png")).absoluteFilePath());
    // Different dir, same name -> untouched
    QCOMPARE(outputPath("out/x.png", QString(), "other", "png"), QStringLiteral("other/x.png"));
}

void TestConversionPlanner::testConverterNameRouting()
{
    QCOMPARE(converterNameFor("mp4", "mkv"), QStringLiteral("FFmpeg"));
    QCOMPARE(converterNameFor("mp3", "flac"), QStringLiteral("FFmpeg"));
    QCOMPARE(converterNameFor("png", "jpg"), QStringLiteral("ImageMagick"));
    QCOMPARE(converterNameFor("docx", "md"), QStringLiteral("Pandoc"));
    // unknown output ext -> input ext decides
    QCOMPARE(converterNameFor("weird", "mp4"), QStringLiteral("FFmpeg"));
}

void TestConversionPlanner::testConverterNameUnknownFormat()
{
    QVERIFY(converterNameFor("weird", "alsoweird").isEmpty());
}

void TestConversionPlanner::testMergeParamsMediaNormalization()
{
    QVariantMap base;
    base["converter"] = "FFmpeg";
    QVariantMap dlg;
    dlg["resolution"] = "1920X1080"; // uppercase x -> lowercase
    dlg["videoBitrate"] = "2M";      // string -> kbps int
    dlg["audioBitrate"] = "";        // empty -> not copied
    dlg["framerate"] = "30";         // framerate -> frameRate int
    dlg["twoPass"] = true;
    const QVariantMap merged = mergeParams(base, dlg, FormatRegistry::Category::Video);
    QCOMPARE(merged.value("resolution").toString(), QStringLiteral("1920x1080"));
    QCOMPARE(merged.value("videoBitrate").toInt(), 2000);
    QVERIFY(!merged.contains("audioBitrate"));
    QCOMPARE(merged.value("frameRate").toInt(), 30);
    QCOMPARE(merged.value("twoPass").toBool(), true);
    QCOMPARE(merged.value("converter").toString(), QStringLiteral("FFmpeg"));
}

void TestConversionPlanner::testMergeParamsDocumentGeometry()
{
    QVariantMap base;
    base["converter"] = "Pandoc";
    QVariantMap dlg;
    dlg["pageSize"] = "a4";
    dlg["orientation"] = "landscape";
    dlg["marginTop"] = 2.5;
    dlg["numberSections"] = true;
    const QVariantMap merged = mergeParams(base, dlg, FormatRegistry::Category::Document);
    const QVariantMap vars = merged.value("variableMap").toMap();
    const QString geo = vars.value("geometry").toString();
    QVERIFY(geo.startsWith("a4,landscape"));
    QVERIFY(geo.contains("top=2.5in"));
    // raw geometry keys must NOT leak into the param map
    QVERIFY(!merged.contains("pageSize"));
    QVERIFY(!merged.contains("orientation"));
    // numberSections -> extraArgs
    const QStringList extra = merged.value("extraArgs").toStringList();
    QVERIFY(extra.contains("--number-sections"));
}

void TestConversionPlanner::testMergeParamsPassthroughForUnknownCategory()
{
    QVariantMap base;
    QVariantMap dlg;
    dlg["resolution"] = "1920X1080";
    dlg["videoBitrate"] = "2M";
    const QVariantMap merged = mergeParams(base, dlg, FormatRegistry::Category::Unknown);
    // No category-specific fixups: values pass through verbatim.
    QCOMPARE(merged.value("resolution").toString(), QStringLiteral("1920X1080"));
    QCOMPARE(merged.value("videoBitrate").toString(), QStringLiteral("2M"));
}
