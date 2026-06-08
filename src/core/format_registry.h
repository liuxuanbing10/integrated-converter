#ifndef FORMAT_REGISTRY_H
#define FORMAT_REGISTRY_H

#include <QStringList>
#include <QSet>
#include <QMap>
#include <QString>

class FormatRegistry {
public:
    enum class Category { Video, Audio, Image, Document, Unknown };
    enum class Converter { FFmpeg, Pandoc, ImageMagick, Unknown };

    static FormatRegistry& instance();

    // ── Format lists ──────────────────────────────────────────────
    const QStringList& videoFormats() const;
    const QStringList& audioFormats() const;
    const QStringList& imageInputFormats() const;
    const QStringList& imageOutputFormats() const;
    const QStringList& documentFormats() const;
    const QStringList& documentInputFormats() const;
    const QStringList& documentOutputFormats() const;
    QStringList allFormats() const;

    // ── Classification helpers ────────────────────────────────────
    Category category(const QString& format) const;
    Converter converterForExt(const QString& format) const;
    bool isVideo(const QString& format) const;
    bool isAudio(const QString& format) const;
    bool isImage(const QString& format) const;
    bool isDocument(const QString& format) const;
    bool isSupported(const QString& format) const;

    // ── FFmpeg lookups ────────────────────────────────────────────
    QString ffmpegFormatName(const QString& ext) const;
    QString ffmpegVideoCodec(const QString& codec) const;
    QString ffmpegAudioCodec(const QString& codec) const;

    // ── Pandoc lookups ────────────────────────────────────────────
    QString pandocFormatName(const QString& ext) const;

    // ── ImageMagick lookups ───────────────────────────────────────
    QString imagemagickFormatName(const QString& ext) const;

    // ── UI helpers ────────────────────────────────────────────────
    QString fileDialogFilter() const;
    QString fileDialogVideoFilter() const;
    QString fileDialogAudioFilter() const;
    QString fileDialogImageFilter() const;
    QString fileDialogDocumentFilter() const;

private:
    FormatRegistry();
    ~FormatRegistry() = default;
    FormatRegistry(const FormatRegistry&) = delete;
    FormatRegistry& operator=(const FormatRegistry&) = delete;

    void init();

    // ── Order-preserving lists (for UI iteration) ─────────────────
    QStringList m_videoFormats;
    QStringList m_audioFormats;
    QStringList m_imageInputFormats;
    QStringList m_imageOutputFormats;
    QStringList m_documentInputFormats;
    QStringList m_documentOutputFormats;

    // ── Sets for O(1) membership tests ────────────────────────────
    QSet<QString> m_videoFormatsSet;
    QSet<QString> m_audioFormatsSet;
    QSet<QString> m_imageInputFormatsSet;
    QSet<QString> m_documentInputFormatsSet;

    QMap<QString, QString> m_ffmpegFormatMap;
    QMap<QString, QString> m_videoCodecMap;
    QMap<QString, QString> m_audioCodecMap;
    QMap<QString, QString> m_pandocFormatMap;
    QMap<QString, QString> m_imagemagickFormatMap;
};

#endif // FORMAT_REGISTRY_H
