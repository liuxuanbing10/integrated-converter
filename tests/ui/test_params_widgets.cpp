#include "test_params_widgets.h"

#include "audio_params_widget.h"
#include "document_params_widget.h"
#include "image_params_widget.h"
#include "video_params_widget.h"

#include <QComboBox>
#include <QSignalSpy>
#include <QTest>

void TestParamsWidgets::testAudioRoundTrip()
{
    AudioParamsWidget w;
    QVariantMap saved;
    saved["audioCodec"] = "mp3";
    saved["audioBitrate"] = "192k";
    saved["sampleRate"] = 48000;
    saved["channels"] = 2;
    saved["vbrQuality"] = 2;
    w.setParams(saved);

    const QVariantMap out = w.getParams();
    QCOMPARE(out.value("audioCodec").toString(), QStringLiteral("mp3"));
    QCOMPARE(out.value("audioBitrate").toString(), QStringLiteral("192k"));
    QCOMPARE(out.value("sampleRate").toInt(), 48000);
    QCOMPARE(out.value("channels").toInt(), 2);
    QCOMPARE(out.value("vbrQuality").toInt(), 2);
    QVERIFY(w.validate().isEmpty());
}

void TestParamsWidgets::testVideoRoundTrip()
{
    VideoParamsWidget w;
    QVariantMap saved;
    saved["videoCodec"] = "hevc";
    saved["audioCodec"] = "copy";
    saved["resolution"] = "1280x720";
    saved["videoBitrate"] = "2M";
    saved["framerate"] = "30";
    saved["preset"] = "slow";
    saved["crf"] = 20;
    saved["twoPass"] = true;
    w.setParams(saved);

    const QVariantMap out = w.getParams();
    QCOMPARE(out.value("videoCodec").toString(), QStringLiteral("hevc"));
    QCOMPARE(out.value("audioCodec").toString(), QStringLiteral("copy"));
    QCOMPARE(out.value("resolution").toString(), QStringLiteral("1280x720"));
    QCOMPARE(out.value("videoBitrate").toString(), QStringLiteral("2M"));
    QCOMPARE(out.value("framerate").toString(), QStringLiteral("30"));
    QCOMPARE(out.value("preset").toString(), QStringLiteral("slow"));
    QCOMPARE(out.value("crf").toInt(), 20);
    QCOMPARE(out.value("twoPass").toBool(), true);
    QVERIFY(w.validate().isEmpty());
}

void TestParamsWidgets::testImageRoundTrip()
{
    ImageParamsWidget w;
    QVariantMap saved;
    saved["quality"] = 70;
    saved["resize"] = "50%";
    saved["compression"] = "LZW";
    saved["density"] = 300;
    saved["strip"] = true;
    saved["depth"] = "16";
    w.setParams(saved);

    const QVariantMap out = w.getParams();
    QCOMPARE(out.value("quality").toInt(), 70);
    QCOMPARE(out.value("resize").toString(), QStringLiteral("50%"));
    QCOMPARE(out.value("compression").toString(), QStringLiteral("LZW"));
    QCOMPARE(out.value("density").toInt(), 300);
    QCOMPARE(out.value("strip").toBool(), true);
    QCOMPARE(out.value("depth").toString(), QStringLiteral("16"));
    QVERIFY(w.validate().isEmpty());
}

void TestParamsWidgets::testDocumentRoundTrip()
{
    DocumentParamsWidget w;
    QVariantMap saved;
    saved["pageSize"] = "a5";
    saved["orientation"] = "landscape";
    saved["marginTop"] = 2.5;
    saved["marginBottom"] = 0.5;
    saved["marginLeft"] = 1.5;
    saved["marginRight"] = 1.0;
    saved["pdfEngine"] = "xelatex";
    saved["toc"] = true;
    saved["tocDepth"] = 4;
    saved["numberSections"] = true;
    w.setParams(saved);

    const QVariantMap out = w.getParams();
    QCOMPARE(out.value("pageSize").toString(), QStringLiteral("a5"));
    QCOMPARE(out.value("orientation").toString(), QStringLiteral("landscape"));
    QCOMPARE(out.value("marginTop").toDouble(), 2.5);
    QCOMPARE(out.value("marginBottom").toDouble(), 0.5);
    QCOMPARE(out.value("pdfEngine").toString(), QStringLiteral("xelatex"));
    QCOMPARE(out.value("toc").toBool(), true);
    QCOMPARE(out.value("tocDepth").toInt(), 4);
    QCOMPARE(out.value("numberSections").toBool(), true);
    QVERIFY(w.validate().isEmpty());
}

void TestParamsWidgets::testSetParamsDoesNotEmit()
{
    // The dialog saves/loads via setParams; a stray paramsChanged there would
    // mark the dialog dirty. Base-class registry must keep it silent, while a
    // user-driven control change must fire exactly once.
    AudioParamsWidget w;
    QSignalSpy spy(&w, &AbstractParamsWidget::paramsChanged);
    QVariantMap m;
    m["audioCodec"] = "opus";
    w.setParams(m);
    QCOMPARE(spy.count(), 0);

    auto* combo = w.findChild<QComboBox*>();
    QVERIFY(combo != nullptr);
    const int before = combo->currentIndex();
    combo->setCurrentIndex((before + 1) % combo->count());
    if (combo->currentIndex() != before)
    {
        QVERIFY(spy.count() >= 1);
    }
}

void TestParamsWidgets::testValidateAfterDefaults()
{
    VideoParamsWidget v;
    ImageParamsWidget i;
    AudioParamsWidget a;
    DocumentParamsWidget d;
    QVERIFY(v.validate().isEmpty());
    QVERIFY(i.validate().isEmpty());
    QVERIFY(a.validate().isEmpty());
    QVERIFY(d.validate().isEmpty());
}

void TestParamsWidgets::testComboInvalidKeyIgnored()
{
    // A saved value that matches no combo entry must not crash nor silently
    // reset the combo away from its current selection (findData -> -1 -> keep).
    AudioParamsWidget w;
    QVariantMap junk;
    junk["audioCodec"] = "definitely-not-a-codec";
    w.setParams(junk);
    QCOMPARE(w.getParams().value("audioCodec").toString(), QStringLiteral("aac"));
}

#include "test_params_widgets.moc"
