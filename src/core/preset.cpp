#include "preset.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QUuid>

namespace
{
const char* categoryKey(FormatRegistry::Category c)
{
    switch (c)
    {
        case FormatRegistry::Category::Image:
            return "image";
        case FormatRegistry::Category::Document:
            return "document";
        case FormatRegistry::Category::Audio:
            return "audio";
        case FormatRegistry::Category::Video:
            return "video";
        default:
            return "unknown";
    }
}

FormatRegistry::Category categoryFromKey(const QString& s)
{
    if (s == QLatin1String("image"))
        return FormatRegistry::Category::Image;
    if (s == QLatin1String("document"))
        return FormatRegistry::Category::Document;
    if (s == QLatin1String("audio"))
        return FormatRegistry::Category::Audio;
    if (s == QLatin1String("video"))
        return FormatRegistry::Category::Video;
    return FormatRegistry::Category::Unknown;
}

const char* engineKey(FormatRegistry::Converter c)
{
    switch (c)
    {
        case FormatRegistry::Converter::FFmpeg:
            return "ffmpeg";
        case FormatRegistry::Converter::Pandoc:
            return "pandoc";
        case FormatRegistry::Converter::ImageMagick:
            return "imagemagick";
        default:
            return "unknown";
    }
}

FormatRegistry::Converter engineFromKey(const QString& s)
{
    if (s == QLatin1String("ffmpeg"))
        return FormatRegistry::Converter::FFmpeg;
    if (s == QLatin1String("pandoc"))
        return FormatRegistry::Converter::Pandoc;
    if (s == QLatin1String("imagemagick"))
        return FormatRegistry::Converter::ImageMagick;
    return FormatRegistry::Converter::Unknown;
}

QString engineToString(FormatRegistry::Converter c)
{
    switch (c)
    {
        case FormatRegistry::Converter::FFmpeg:
            return QStringLiteral("FFmpeg");
        case FormatRegistry::Converter::Pandoc:
            return QStringLiteral("Pandoc");
        case FormatRegistry::Converter::ImageMagick:
            return QStringLiteral("ImageMagick");
        default:
            return QString();
    }
}

constexpr int kShareMarker = 1;
} // namespace

QJsonObject Preset::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("schemaVersion")] = schemaVersion;
    o[QStringLiteral("id")] = id;
    o[QStringLiteral("name")] = name;
    o[QStringLiteral("category")] = QString::fromLatin1(categoryKey(category));
    o[QStringLiteral("engine")] = QString::fromLatin1(engineKey(engine));
    o[QStringLiteral("format")] = format;
    o[QStringLiteral("params")] = QJsonValue::fromVariant(params);
    if (createdAt.isValid())
    {
        o[QStringLiteral("createdAt")] = createdAt.toString(Qt::ISODate);
    }
    return o;
}

QVariantMap Preset::normalizeParams(const QVariantMap& raw)
{
    QVariantMap out;
    for (auto it = raw.begin(); it != raw.end(); ++it)
    {
        const QVariant v = it.value();
        if (v.typeId() == QMetaType::LongLong && v.toLongLong() <= Q_INT64_C(2147483647) &&
            v.toLongLong() >= Q_INT64_C(-2147483648))
        {
            out[it.key()] = v.toInt(); // JSON ints come back as qlonglong
            continue;
        }
        if (v.canConvert<QVariantList>())
        {
            const QVariantList list = v.toList();
            bool allStrings = !list.isEmpty();
            QStringList strs;
            for (const QVariant& e : list)
            {
                if (e.typeId() != QMetaType::QString)
                {
                    allStrings = false;
                    break;
                }
                strs << e.toString();
            }
            if (allStrings)
            {
                out[it.key()] = strs; // QVariantList of strings -> QStringList
                continue;
            }
        }
        out[it.key()] = v;
    }
    return out;
}

Preset Preset::fromJson(const QJsonObject& obj)
{
    Preset p;
    p.schemaVersion = obj.value(QStringLiteral("schemaVersion")).toInt(1);
    p.id = obj.value(QStringLiteral("id")).toString();
    p.name = obj.value(QStringLiteral("name")).toString();
    p.category = categoryFromKey(obj.value(QStringLiteral("category")).toString());
    p.engine = engineFromKey(obj.value(QStringLiteral("engine")).toString());
    p.format = obj.value(QStringLiteral("format")).toString().toLower();
    p.params = normalizeParams(obj.value(QStringLiteral("params")).toVariant().toMap());
    const QString when = obj.value(QStringLiteral("createdAt")).toString();
    if (!when.isEmpty())
    {
        p.createdAt = QDateTime::fromString(when, Qt::ISODate);
    }
    return p;
}

QString Preset::validate() const
{
    if (name.trimmed().isEmpty())
    {
        return QStringLiteral("preset name is empty");
    }
    if (format.trimmed().isEmpty())
    {
        return QStringLiteral("preset target format is empty");
    }
    if (engine == FormatRegistry::Converter::Unknown)
    {
        return QStringLiteral("preset engine is unknown");
    }
    // Engine/category consistency: the routed converter for the target ext
    // must agree with the stored engine (catches hand-edited share files).
    const QString routed = engineToString(FormatRegistry::instance().converterForExt(format));
    if (!routed.isEmpty() && routed != engineToString(engine))
    {
        return QStringLiteral("preset engine (%1) disagrees with format routing (%2 for .%3)")
            .arg(engineToString(engine), routed, format);
    }
    return QString();
}

QJsonObject Preset::toShareJson() const
{
    QJsonObject o = toJson();
    o[QStringLiteral("presetFormat")] = kShareMarker;
    return o;
}

bool Preset::fromShareJson(const QJsonObject& obj, Preset* out, QString* reason)
{
    const int marker = obj.value(QStringLiteral("presetFormat")).toInt(0);
    if (marker != kShareMarker)
    {
        if (reason)
        {
            *reason = QStringLiteral("not a preset share file (missing presetFormat marker)");
        }
        return false;
    }
    Preset p = fromJson(obj);
    const QString bad = p.validate();
    if (!bad.isEmpty())
    {
        if (reason)
        {
            *reason = bad;
        }
        return false;
    }
    if (p.id.isEmpty())
    {
        p.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    *out = p;
    return true;
}

// ── PresetLibrary ───────────────────────────────────────────────────────────

PresetLibrary::PresetLibrary(const QString& dirPath) : m_dir(dirPath)
{
    QDir().mkpath(m_dir);
}

QString PresetLibrary::safeFileName(const QString& id)
{
    // ids are uuid-ish; still scrub anything path-y (defense vs hand-edited).
    // Hand-rolled filter — QRegularExpression would drag in Qt6Core5Compat.
    QString s;
    s.reserve(id.size());
    for (const QChar c : id)
    {
        const bool ok =
            (c >= QLatin1Char('a') && c <= QLatin1Char('z')) || (c >= QLatin1Char('A') && c <= QLatin1Char('Z')) ||
            (c >= QLatin1Char('0') && c <= QLatin1Char('9')) || c == QLatin1Char('_') || c == QLatin1Char('-');
        s.append(ok ? c : QLatin1Char('_'));
    }
    return s;
}

bool PresetLibrary::save(const Preset& preset, QString* error)
{
    const QString bad = preset.validate();
    if (!bad.isEmpty())
    {
        if (error)
            *error = bad;
        return false;
    }
    const QString path = QDir(m_dir).filePath(safeFileName(preset.id) + QStringLiteral(".json"));
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly))
    {
        if (error)
            *error = QStringLiteral("cannot open %1 for writing").arg(path);
        return false;
    }
    f.write(QJsonDocument(preset.toJson()).toJson(QJsonDocument::Indented));
    if (!f.commit())
    {
        if (error)
            *error = QStringLiteral("failed to save %1").arg(path);
        return false;
    }
    return true;
}

bool PresetLibrary::remove(const QString& id, QString* error)
{
    const QString path = QDir(m_dir).filePath(safeFileName(id) + QStringLiteral(".json"));
    if (!QFile::remove(path))
    {
        if (error)
            *error = QStringLiteral("no such preset file: %1").arg(path);
        return false;
    }
    return true;
}

bool PresetLibrary::load(const QString& id, Preset* out, QString* error) const
{
    const QString path = QDir(m_dir).filePath(safeFileName(id) + QStringLiteral(".json"));
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
    {
        if (error)
            *error = QStringLiteral("cannot read %1").arg(path);
        return false;
    }
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject())
    {
        if (error)
            *error = QStringLiteral("invalid json in %1: %2").arg(path, pe.errorString());
        return false;
    }
    *out = Preset::fromJson(doc.object());
    return true;
}

QList<Preset> PresetLibrary::all() const
{
    QList<Preset> out;
    QDir dir(m_dir);
    const QFileInfoList entries = dir.entryInfoList({QStringLiteral("*.json")}, QDir::Files, QDir::Name);
    for (const QFileInfo& fi : entries)
    {
        Preset p;
        QString err;
        // Key on the file's base name — save() names files by id, so this is
        // the id unless the user renamed the file by hand (then base name is
        // the load key regardless).
        if (!load(fi.completeBaseName(), &p, &err))
        {
            continue; // corrupt/skipped
        }
        out << p;
    }
    return out;
}

bool PresetLibrary::exportTo(const Preset& preset, const QString& filePath, QString* error)
{
    QSaveFile f(filePath);
    if (!f.open(QIODevice::WriteOnly))
    {
        if (error)
            *error = QStringLiteral("cannot open export target %1").arg(filePath);
        return false;
    }
    f.write(QJsonDocument(preset.toShareJson()).toJson(QJsonDocument::Indented));
    if (!f.commit())
    {
        if (error)
            *error = QStringLiteral("failed to write export %1").arg(filePath);
        return false;
    }
    return true;
}

bool PresetLibrary::importFrom(const QString& filePath, Preset* out, QString* error)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
    {
        if (error)
            *error = QStringLiteral("cannot read %1").arg(filePath);
        return false;
    }
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject())
    {
        if (error)
            *error = QStringLiteral("invalid json: %1").arg(pe.errorString());
        return false;
    }
    Preset p;
    QString reason;
    if (!Preset::fromShareJson(doc.object(), &p, &reason))
    {
        if (error)
            *error = reason;
        return false;
    }
    if (!save(p, error))
    {
        return false;
    }
    *out = p;
    return true;
}
