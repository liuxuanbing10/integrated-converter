#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H
#include <QObject>
#include <QString>
#include <QVariantMap>
class ConfigManager : public QObject
{
    Q_OBJECT
public:
    static ConfigManager& instance();
    /// Resolve ffmpeg/ffprobe/pandoc/imagemagick paths, but ONLY when a
    /// stored value is missing/bare/Shared-stale. Memoized per session:
    /// steady-state startups cost four cheap existence checks, not two
    /// process-wide candidate scans with `--version` probes (analysis §7.1).
    /// Called by the converters at construction and by loadConfig after a
    /// config file wholesale-replaces m_config.
    void ensureToolPaths();
    bool loadConfig(const QString& filePath);
    bool saveConfig(const QString& filePath);
    QVariant value(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setValue(const QString& key, const QVariant& value);
    int maxParallelTasks() const;
    void setMaxParallelTasks(int count);
    QString outputDirectory() const;
    void setOutputDirectory(const QString& dir);
    int logLevel() const;
    void setLogLevel(int level);
    QVariantMap allConfig() const
    {
        return m_config;
    }

private:
    ConfigManager();
    ~ConfigManager();
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    void initDefaultConfig();
    QString findExecutable(const QString& name);
    bool toolPathUsable(const QString& key, const QString& exeName) const;
    /// Skip detection entirely when the stored value is already usable.
    void detectTool(const QString& key, const QString& exeName);
    void detectFFmpegPath();
    void detectFFprobePath();
    void detectPandocPath();
    void detectImageMagickPath();
    QVariantMap m_config;
    bool m_toolsResolved = false;
    QString m_configFilePath;
};
#endif // CONFIG_MANAGER_H
