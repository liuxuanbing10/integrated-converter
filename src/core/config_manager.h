#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H
#include <QObject>
#include <QVariantMap>
#include <QString>
class ConfigManager : public QObject {
    Q_OBJECT
public:
    static ConfigManager& instance();
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
    QVariantMap allConfig() const { return m_config; }
private:
    ConfigManager();
    ~ConfigManager();
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    void initDefaultConfig();
    QString findExecutable(const QString& name);
    void detectFFmpegPath();
    void detectPandocPath();
    void detectImageMagickPath();
    QVariantMap m_config;
    QString m_configFilePath;
};
#endif // CONFIG_MANAGER_H
