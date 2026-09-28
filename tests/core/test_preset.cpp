#include "test_preset.h"

#include "../../src/core/preset.h"

#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTest>

static Preset samplePreset()
{
    Preset p;
    p.id = QStringLiteral("abcd-1234");
    p.name = QStringLiteral("gif 转 mp4 顺滑");
    // category must agree with converterForExt("mp4") == FFmpeg -> a
    // media category (Video), never Document (validate() enforces routing).
    p.category = FormatRegistry::Category::Video;
    p.engine = FormatRegistry::Converter::FFmpeg;
    p.format = QStringLiteral("mp4");
    p.params[QStringLiteral("videoCodec")] = QStringLiteral("libx264");
    p.params[QStringLiteral("crf")] = 23;
    p.params[QStringLiteral("extraArgs")] = QStringList{QStringLiteral("-movflags"), QStringLiteral("+faststart")};
    p.createdAt = QDateTime::currentDateTimeUtc();
    return p;
}

void TestPreset::testJsonRoundTripFullEquality()
{
    const Preset p = samplePreset();
    const Preset q = Preset::fromJson(p.toJson());
    QCOMPARE(q.schemaVersion, p.schemaVersion);
    QCOMPARE(q.id, p.id);
    QCOMPARE(q.name, p.name);
    QCOMPARE(q.category, p.category);
    QCOMPARE(q.engine, p.engine);
    QCOMPARE(q.format, p.format);
    QCOMPARE(q.params, p.params); // params 全等 — acceptance line
    // ISODate round-trip keeps seconds; compare serialized form, not ms-precision
    // QDateTime equality (that would flake whenever ms != 0).
    QCOMPARE(q.createdAt.toString(Qt::ISODate), p.createdAt.toString(Qt::ISODate));
}

void TestPreset::testLibrarySaveLoadRemoveRoundTrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    PresetLibrary lib(dir.path());

    const Preset p = samplePreset();
    QString err;
    QVERIFY2(lib.save(p, &err), qPrintable(err));

    Preset loaded;
    QVERIFY(lib.load(p.id, &loaded, &err));
    QCOMPARE(loaded.params, p.params);
    QCOMPARE(loaded.name, p.name);

    QList<Preset> all = lib.all();
    QCOMPARE(all.size(), 1);

    QVERIFY(lib.remove(p.id, &err));
    QVERIFY(!lib.load(p.id, &loaded, &err));
    QVERIFY(lib.all().isEmpty());
    // removing twice = failure with message, no crash
    QVERIFY(!lib.remove(p.id, &err));
    QVERIFY(!err.isEmpty());
}

void TestPreset::testValidateRejectsBadPresets()
{
    Preset p = samplePreset();
    QVERIFY(p.validate().isEmpty());

    p.name = QStringLiteral("  ");
    QVERIFY(p.validate().contains(QStringLiteral("name")));
    p = samplePreset();

    p.format.clear();
    QVERIFY(p.validate().contains(QStringLiteral("format")));
    p = samplePreset();

    p.engine = FormatRegistry::Converter::Unknown;
    QVERIFY(p.validate().contains(QStringLiteral("engine")));

    // hand-edited share file claiming pandoc for .mp4 -> routing mismatch
    p.engine = FormatRegistry::Converter::Pandoc;
    QVERIFY(!p.validate().isEmpty());
}

void TestPreset::testShareMarkerRejectsForeignJson()
{
    // A plain config.json body must be rejected as a preset import.
    QJsonObject foreign;
    foreign[QStringLiteral("maxParallelTasks")] = 4;
    Preset out;
    QString reason;
    QVERIFY(!Preset::fromShareJson(foreign, &out, &reason));
    QVERIFY(reason.contains(QStringLiteral("marker")));

    // ours passes and gets an id when missing
    Preset p = samplePreset();
    p.id.clear();
    Preset in;
    QVERIFY(Preset::fromShareJson(p.toShareJson(), &in, &reason));
    QVERIFY(!in.id.isEmpty());
}

void TestPreset::testShareFileExportImportRoundTrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    PresetLibrary lib(dir.path() + QStringLiteral("/lib"));
    const Preset p = samplePreset();
    const QString sharePath = dir.path() + QStringLiteral("/share.json");

    QString err;
    QVERIFY2(lib.exportTo(p, sharePath, &err), qPrintable(err));
    Preset imported;
    QVERIFY2(lib.importFrom(sharePath, &imported, &err), qPrintable(err));
    QCOMPARE(imported.params, p.params);
    QCOMPARE(imported.id, p.id); // id preserved so round-trips compare equal
    // import also filed it into the library
    Preset fromLib;
    QVERIFY(lib.load(imported.id, &fromLib, &err));
    QCOMPARE(fromLib.name, p.name);
}

void TestPreset::testSchemaVersionSurvivesUnknownFields()
{
    // Forward-compat: a v2-era file with unknown keys must still load the
    // v1-known fields (migration guard = schemaVersion round-trips).
    QJsonObject o = samplePreset().toJson();
    o[QStringLiteral("futureField")] = QStringLiteral("whatever");
    o[QStringLiteral("schemaVersion")] = 2;
    const Preset p = Preset::fromJson(o);
    QCOMPARE(p.schemaVersion, 2);
    QCOMPARE(p.name, samplePreset().name);
    QCOMPARE(p.params, samplePreset().params);
}
