#include "format_registry.h"

FormatRegistry& FormatRegistry::instance() {
    static FormatRegistry registry;
    return registry;
}

FormatRegistry::FormatRegistry() {
    init();
}

void FormatRegistry::init() {
    // ── Video ─────────────────────────────────────────────────────
    m_videoFormats << "mp4" << "avi" << "mkv" << "mov" << "wmv"
                   << "flv" << "webm" << "mpeg";

    // ── Audio ─────────────────────────────────────────────────────
    m_audioFormats << "mp3" << "wav" << "flac" << "aac" << "ogg"
                   << "m4a" << "wma";

    // ── Document (input) ──────────────────────────────────────────
    m_documentInputFormats << "md" << "markdown" << "html" << "htm"
                           << "tex" << "latex"
                           << "docx" << "word"
                           << "rst" << "rest"
                           << "org" << "epub"
                           << "txt" << "plain"
                           << "odt" << "csv" << "json";

    // ── Document (output) ─────────────────────────────────────────
    m_documentOutputFormats << "md" << "markdown" << "html" << "htm"
                            << "tex" << "latex"
                            << "docx" << "word" << "pdf"
                            << "rst" << "rest"
                            << "org" << "epub" << "pptx"
                            << "txt" << "plain"
                            << "odt" << "json";

    // ── FFmpeg format mapping ─────────────────────────────────────
    m_ffmpegFormatMap["mp4"]  = "mp4";
    m_ffmpegFormatMap["avi"]  = "avi";
    m_ffmpegFormatMap["flv"]  = "flv";
    m_ffmpegFormatMap["mkv"]  = "matroska";
    m_ffmpegFormatMap["webm"] = "webm";
    m_ffmpegFormatMap["mov"]  = "mov";
    m_ffmpegFormatMap["wmv"]  = "asf";
    m_ffmpegFormatMap["mpeg"] = "mpeg";
    m_ffmpegFormatMap["mp3"]  = "mp3";
    m_ffmpegFormatMap["wav"]  = "wav";
    m_ffmpegFormatMap["aac"]  = "adts";
    m_ffmpegFormatMap["flac"] = "flac";
    m_ffmpegFormatMap["ogg"]  = "ogg";
    m_ffmpegFormatMap["m4a"]  = "mp4";

    // ── FFmpeg video codec mapping ────────────────────────────────
    m_videoCodecMap["h264"]  = "libx264";
    m_videoCodecMap["h265"]  = "libx265";
    m_videoCodecMap["hevc"]  = "libx265";
    m_videoCodecMap["vp9"]   = "libvpx-vp9";
    m_videoCodecMap["vp8"]   = "libvpx";
    m_videoCodecMap["av1"]   = "libaom-av1";
    m_videoCodecMap["mpeg4"] = "mpeg4";
    m_videoCodecMap["mpeg2"] = "mpeg2video";

    // ── FFmpeg audio codec mapping ────────────────────────────────
    m_audioCodecMap["aac"]    = "aac";
    m_audioCodecMap["mp3"]    = "libmp3lame";
    m_audioCodecMap["opus"]   = "libopus";
    m_audioCodecMap["vorbis"] = "libvorbis";
    m_audioCodecMap["flac"]   = "flac";
    m_audioCodecMap["pcm"]    = "pcm_s16le";

    // ── Pandoc format mapping ─────────────────────────────────────
    m_pandocFormatMap["md"]       = "markdown";
    m_pandocFormatMap["markdown"] = "markdown";
    m_pandocFormatMap["html"]     = "html";
    m_pandocFormatMap["htm"]      = "html";
    m_pandocFormatMap["tex"]      = "latex";
    m_pandocFormatMap["latex"]    = "latex";
    m_pandocFormatMap["docx"]     = "docx";
    m_pandocFormatMap["word"]     = "docx";
    m_pandocFormatMap["pdf"]      = "pdf";
    m_pandocFormatMap["rst"]      = "rst";
    m_pandocFormatMap["rest"]     = "rst";
    m_pandocFormatMap["org"]      = "org";
    m_pandocFormatMap["epub"]     = "epub";
    m_pandocFormatMap["pptx"]     = "pptx";
    m_pandocFormatMap["txt"]      = "plain";
    m_pandocFormatMap["plain"]    = "plain";
    m_pandocFormatMap["odt"]      = "odt";
    m_pandocFormatMap["csv"]      = "csv";
    m_pandocFormatMap["json"]     = "json";
}

// ── Format lists ─────────────────────────────────────────────────
const QStringList& FormatRegistry::videoFormats() const {
    return m_videoFormats;
}

const QStringList& FormatRegistry::audioFormats() const {
    return m_audioFormats;
}

const QStringList& FormatRegistry::documentFormats() const {
    return m_documentInputFormats;
}

const QStringList& FormatRegistry::documentInputFormats() const {
    return m_documentInputFormats;
}

const QStringList& FormatRegistry::documentOutputFormats() const {
    return m_documentOutputFormats;
}

QStringList FormatRegistry::allFormats() const {
    return m_videoFormats + m_audioFormats + m_documentInputFormats;
}

// ── Classification ───────────────────────────────────────────────
FormatRegistry::Category FormatRegistry::category(const QString& format) const {
    if (isVideo(format)) return Category::Video;
    if (isAudio(format)) return Category::Audio;
    if (isDocument(format)) return Category::Document;
    return Category::Unknown;
}

FormatRegistry::Converter FormatRegistry::converterForExt(const QString& format) const {
    if (isVideo(format) || isAudio(format)) return Converter::FFmpeg;
    if (isDocument(format)) return Converter::Pandoc;
    return Converter::Unknown;
}

bool FormatRegistry::isVideo(const QString& format) const {
    return m_videoFormats.contains(format.toLower());
}

bool FormatRegistry::isAudio(const QString& format) const {
    return m_audioFormats.contains(format.toLower());
}

bool FormatRegistry::isDocument(const QString& format) const {
    return m_documentInputFormats.contains(format.toLower());
}

bool FormatRegistry::isSupported(const QString& format) const {
    return isVideo(format) || isAudio(format) || isDocument(format);
}

// ── FFmpeg lookups ───────────────────────────────────────────────
QString FormatRegistry::ffmpegFormatName(const QString& ext) const {
    return m_ffmpegFormatMap.value(ext.toLower(), ext.toLower());
}

QString FormatRegistry::ffmpegVideoCodec(const QString& codec) const {
    return m_videoCodecMap.value(codec.toLower(), codec);
}

QString FormatRegistry::ffmpegAudioCodec(const QString& codec) const {
    return m_audioCodecMap.value(codec.toLower(), codec);
}

// ── Pandoc lookups ───────────────────────────────────────────────
QString FormatRegistry::pandocFormatName(const QString& ext) const {
    return m_pandocFormatMap.value(ext.toLower(), ext.toLower());
}

// ── UI helpers ───────────────────────────────────────────────────
QString FormatRegistry::fileDialogFilter() const {
    QStringList allExts = allFormats();
    QStringList prefixed;
    for (const QString& e : allExts) {
        prefixed << ("*." + e);
    }
    return QString("All Supported Formats (%1)").arg(prefixed.join(" "));
}

static QString joinExts(const QStringList& fmts) {
    QStringList prefixed;
    for (const QString& e : fmts) {
        prefixed << ("*." + e);
    }
    return prefixed.join(" ");
}

QString FormatRegistry::fileDialogVideoFilter() const {
    return QString("Video Files (%1)").arg(joinExts(m_videoFormats));
}

QString FormatRegistry::fileDialogAudioFilter() const {
    return QString("Audio Files (%1)").arg(joinExts(m_audioFormats));
}

QString FormatRegistry::fileDialogDocumentFilter() const {
    return QString("Document Files (%1)").arg(joinExts(m_documentInputFormats));
}
