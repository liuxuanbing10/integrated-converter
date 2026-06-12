#include "test_format_registry.h"
#include "core/format_registry.h"
#include <QTest>

void TestFormatRegistry::testSingletonInstance() {
    FormatRegistry& r1 = FormatRegistry::instance();
    FormatRegistry& r2 = FormatRegistry::instance();
    QVERIFY(&r1 == &r2);
}

void TestFormatRegistry::testIsVideo() {
    FormatRegistry& reg = FormatRegistry::instance();
    QVERIFY(reg.isVideo("mp4"));
    QVERIFY(reg.isVideo("avi"));
    QVERIFY(reg.isVideo("mkv"));
    QVERIFY(reg.isVideo("MOV"));
    QVERIFY(!reg.isVideo("mp3"));
    QVERIFY(!reg.isVideo("txt"));
}

void TestFormatRegistry::testIsAudio() {
    FormatRegistry& reg = FormatRegistry::instance();
    QVERIFY(reg.isAudio("mp3"));
    QVERIFY(reg.isAudio("wav"));
    QVERIFY(reg.isAudio("FLAC"));
    QVERIFY(!reg.isAudio("mp4"));
    QVERIFY(!reg.isAudio("png"));
}

void TestFormatRegistry::testIsImage() {
    FormatRegistry& reg = FormatRegistry::instance();
    QVERIFY(reg.isImage("png"));
    QVERIFY(reg.isImage("jpg"));
    QVERIFY(reg.isImage("JPEG"));
    QVERIFY(!reg.isImage("mp4"));
    QVERIFY(!reg.isImage("txt"));
}

void TestFormatRegistry::testIsDocument() {
    FormatRegistry& reg = FormatRegistry::instance();
    QVERIFY(reg.isDocument("md"));
    QVERIFY(reg.isDocument("txt"));
    QVERIFY(reg.isDocument("html"));
    QVERIFY(!reg.isDocument("mp4"));
    QVERIFY(!reg.isDocument("png"));
}

void TestFormatRegistry::testIsSupported() {
    FormatRegistry& reg = FormatRegistry::instance();
    QVERIFY(reg.isSupported("mp4"));
    QVERIFY(reg.isSupported("mp3"));
    QVERIFY(reg.isSupported("png"));
    QVERIFY(reg.isSupported("md"));
    QVERIFY(!reg.isSupported("xyz"));
    QVERIFY(!reg.isSupported(""));
}

void TestFormatRegistry::testCategory() {
    FormatRegistry& reg = FormatRegistry::instance();
    QCOMPARE(reg.category("mp4"), FormatRegistry::Category::Video);
    QCOMPARE(reg.category("mp3"), FormatRegistry::Category::Audio);
    QCOMPARE(reg.category("png"), FormatRegistry::Category::Image);
    QCOMPARE(reg.category("md"), FormatRegistry::Category::Document);
    QCOMPARE(reg.category("xyz"), FormatRegistry::Category::Unknown);
}

void TestFormatRegistry::testConverterForExt() {
    FormatRegistry& reg = FormatRegistry::instance();
    QCOMPARE(reg.converterForExt("mp4"), FormatRegistry::Converter::FFmpeg);
    QCOMPARE(reg.converterForExt("mp3"), FormatRegistry::Converter::FFmpeg);
    QCOMPARE(reg.converterForExt("png"), FormatRegistry::Converter::ImageMagick);
    QCOMPARE(reg.converterForExt("md"), FormatRegistry::Converter::Pandoc);
    QCOMPARE(reg.converterForExt("xyz"), FormatRegistry::Converter::Unknown);
}

void TestFormatRegistry::testVideoFormats() {
    FormatRegistry& reg = FormatRegistry::instance();
    QVERIFY(reg.videoFormats().contains("mp4"));
    QVERIFY(reg.videoFormats().contains("mkv"));
    QVERIFY(!reg.videoFormats().isEmpty());
}

void TestFormatRegistry::testAudioFormats() {
    FormatRegistry& reg = FormatRegistry::instance();
    QVERIFY(reg.audioFormats().contains("mp3"));
    QVERIFY(reg.audioFormats().contains("wav"));
    QVERIFY(!reg.audioFormats().isEmpty());
}

void TestFormatRegistry::testAllFormats() {
    FormatRegistry& reg = FormatRegistry::instance();
    QStringList all = reg.allFormats();
    QVERIFY(!all.isEmpty());
    QVERIFY(all.contains("mp4"));
    QVERIFY(all.contains("mp3"));
    QVERIFY(all.contains("png"));
    QVERIFY(all.contains("md"));
}

void TestFormatRegistry::testFFmpegFormatName() {
    FormatRegistry& reg = FormatRegistry::instance();
    QCOMPARE(reg.ffmpegFormatName("mp4"), QString("mp4"));
    QCOMPARE(reg.ffmpegFormatName("mkv"), QString("matroska"));
    QCOMPARE(reg.ffmpegFormatName("wmv"), QString("asf"));
    QCOMPARE(reg.ffmpegFormatName("MP4"), QString("mp4"));
}

void TestFormatRegistry::testFFmpegVideoCodec() {
    FormatRegistry& reg = FormatRegistry::instance();
    QCOMPARE(reg.ffmpegVideoCodec("h264"), QString("libx264"));
    QCOMPARE(reg.ffmpegVideoCodec("hevc"), QString("libx265"));
    QCOMPARE(reg.ffmpegVideoCodec("vp9"), QString("libvpx-vp9"));
    QCOMPARE(reg.ffmpegVideoCodec("unknown"), QString("unknown"));
}

void TestFormatRegistry::testFFmpegAudioCodec() {
    FormatRegistry& reg = FormatRegistry::instance();
    QCOMPARE(reg.ffmpegAudioCodec("aac"), QString("aac"));
    QCOMPARE(reg.ffmpegAudioCodec("mp3"), QString("libmp3lame"));
    QCOMPARE(reg.ffmpegAudioCodec("flac"), QString("flac"));
}

void TestFormatRegistry::testPandocFormatName() {
    FormatRegistry& reg = FormatRegistry::instance();
    QCOMPARE(reg.pandocFormatName("md"), QString("markdown"));
    QCOMPARE(reg.pandocFormatName("html"), QString("html"));
    QCOMPARE(reg.pandocFormatName("tex"), QString("latex"));
    QCOMPARE(reg.pandocFormatName("docx"), QString("docx"));
}

void TestFormatRegistry::testImagemagickFormatName() {
    FormatRegistry& reg = FormatRegistry::instance();
    QCOMPARE(reg.imagemagickFormatName("png"), QString("png"));
    QCOMPARE(reg.imagemagickFormatName("jpg"), QString("jpeg"));
    QCOMPARE(reg.imagemagickFormatName("jpeg"), QString("jpeg"));
    QCOMPARE(reg.imagemagickFormatName("tiff"), QString("tiff"));
}

void TestFormatRegistry::testFileDialogFilter() {
    FormatRegistry& reg = FormatRegistry::instance();
    QString filter = reg.fileDialogFilter();
    QVERIFY(!filter.isEmpty());
    QVERIFY(filter.startsWith("All Supported Formats"));
}
