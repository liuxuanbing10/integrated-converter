#ifndef PRESET_H
#define PRESET_H

#include "format_registry.h"

#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include <QVariantMap>

/// §一.1 预设系统 v1 — the "share a .json = share a solution" contract.
/// A Preset is a NAMED, SERIALIZABLE conversion recipe: engine + target
/// format + the exact params ConversionPlanner consumes. Keeping this in
/// core/ (no widgets) means GUI, CLI and the future web shell all speak the
/// same file — the strategy's "两条战线唯一连接点".
struct Preset
{
    int schemaVersion = 1; // migration guard
    QString id;            // stable uuid-ish
    QString name;          // user-visible ("gif 转 mp4 顺滑")
    FormatRegistry::Category category = FormatRegistry::Category::Unknown;
    FormatRegistry::Converter engine = FormatRegistry::Converter::Unknown;
    QString format;     // target extension, lower, no dot
    QVariantMap params; // exact keys converters consume
    QDateTime createdAt;

    /// JSON <-> Preset (the share file IS this object plus a wrapper header).
    QJsonObject toJson() const;
    static Preset fromJson(const QJsonObject& obj);

    /// Params round-tripped through QJsonDocument lose variant typing
    /// (int -> qlonglong, QStringList -> QVariantList). Converters read via
    /// toInt()/toStringList() so either shape works, but the save->load
    /// equality CONTRACT (§一.1 验收: params 全等) demands one canonical
    /// shape: numeric-looking scalars -> int, string-lists -> QStringList.
    static QVariantMap normalizeParams(const QVariantMap& raw);

    /// Validate invariants: name/engine/format present, category<->engine
    /// consistency via FormatRegistry routing. Returns error text, empty = ok.
    QString validate() const;

    /// Single-preset share file: { "presetFormat": 1, ...preset } — the extra
    /// key marks the file type so load-from-disk can reject a plain config.json.
    QJsonObject toShareJson() const;
    /// true + out preset when the object carries the share marker; error text
    /// in `reason` otherwise.
    static bool fromShareJson(const QJsonObject& obj, Preset* out, QString* reason);
};

/// Filesystem-backed library over the app data dir (<dataDir>/presets/*.json),
/// one file per preset = trivially copy/share/diff. All operations are
/// idempotent and never throw; failures return false + error text.
class PresetLibrary
{
public:
    explicit PresetLibrary(const QString& dirPath);

    QString dir() const
    {
        return m_dir;
    }

    bool save(const Preset& preset, QString* error = nullptr);
    bool remove(const QString& id, QString* error = nullptr);
    bool load(const QString& id, Preset* out, QString* error = nullptr) const;

    /// Load every preset file in the dir (invalid files skipped, ids unique).
    QList<Preset> all() const;

    /// Export one preset to an arbitrary path (share file format).
    bool exportTo(const Preset& preset, const QString& filePath, QString* error = nullptr);
    /// Import one share file into the library (gets a fresh id on collision? NO
    /// — id preserved so round-trips compare equal; collision = overwrite).
    bool importFrom(const QString& filePath, Preset* out, QString* error = nullptr);

    static QString safeFileName(const QString& id);

private:
    QString m_dir;
};

#endif // PRESET_H
