#include "config_manager.h"
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QProcess>
#include <QCoreApplication>
#include "logger.h"
ConfigManager& ConfigManager::instance() {
    static ConfigManager instance;
    return instance;
}
ConfigManager::ConfigManager() {
    initDefaultConfig();
    detectFFmpegPath();
    detectPandocPath();
    detectImageMagickPath();
}
ConfigManager::~ConfigManager() {
}
void ConfigManager::initDefaultConfig() {
    m_config["maxParallelTasks"] = 4;
    m_config["outputDirectory"] = QDir::homePath();
    m_config["logLevel"] = 1;
    m_config["autoStartConversion"] = false;
    m_config["overwriteExisting"] = false;
    m_config["showNotification"] = true;
    m_config["ffmpegPath"] = "ffmpeg";
    m_config["pandocPath"] = "pandoc";
    m_config["imagemagickPath"] = "magick";
}
QString ConfigManager::findExecutable(const QString& name) {
    QStringList possiblePaths;
    possiblePaths << name;
    possiblePaths << QDir::homePath() + "/bin/" + name;
    possiblePaths << QDir::rootPath() + "Program Files/ffmpeg/bin/" + name + ".exe";
    possiblePaths << QDir::rootPath() + "Program Files (x86)/ffmpeg/bin/" + name + ".exe";
    possiblePaths << QCoreApplication::applicationDirPath() + "/" + name;
    possiblePaths << QCoreApplication::applicationDirPath() + "/tools/" + name;
    for (const QString& path : possiblePaths) {
        QFileInfo fi(path);
        if (fi.exists() && fi.isExecutable()) {
            return path;
        }
        // 用 --version 验证可执行文件，等待完成再销毁 QProcess，避免 "Destroyed while process running" 警告
        QProcess p;
        p.start(path, QStringList() << "--version");
        if (p.waitForStarted(3000) && p.waitForFinished(5000)) {
            return path;
        }
    }
    return name;
}
void ConfigManager::detectFFmpegPath() {
    QString detected = findExecutable("ffmpeg");
    if (detected != "ffmpeg") {
        m_config["ffmpegPath"] = detected;
        LOG_INFO("ConfigManager", QString("自动检测到FFmpeg路径: %1").arg(detected));
    }
}
void ConfigManager::detectPandocPath() {
    QString detected = findExecutable("pandoc");
    if (detected != "pandoc") {
        m_config["pandocPath"] = detected;
        LOG_INFO("ConfigManager", QString("自动检测到Pandoc路径: %1").arg(detected));
    }
}
void ConfigManager::detectImageMagickPath() {
    // ImageMagick 7+ uses 'magick', IM6 uses 'convert'
    QString detected = findExecutable("magick");
    if (detected == "magick") {
        // magick not found on PATH, try 'convert' for ImageMagick 6
        detected = findExecutable("convert");
        if (detected != "convert") {
            m_config["imagemagickPath"] = detected;
            LOG_INFO("ConfigManager", QString("自动检测到ImageMagick(convert)路径: %1").arg(detected));
        }
    } else {
        m_config["imagemagickPath"] = detected;
        LOG_INFO("ConfigManager", QString("自动检测到ImageMagick(magick)路径: %1").arg(detected));
    }
}
bool ConfigManager::loadConfig(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_WARNING("ConfigManager", QString("无法打开配置文件: %1").arg(filePath));
        return false;
    }
    QByteArray data = file.readAll();
    file.close();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        LOG_ERROR("ConfigManager", QString("配置文件解析错误: %1").arg(error.errorString()));
        return false;
    }
    if (!doc.isObject()) {
        LOG_ERROR("ConfigManager", "配置文件格式错误: 根元素不是对象");
        return false;
    }
    m_config = doc.object().toVariantMap();
    m_configFilePath = filePath;
    LOG_INFO("ConfigManager", QString("配置已加载: %1").arg(filePath));
    return true;
}
bool ConfigManager::saveConfig(const QString& filePath) {
    QJsonDocument doc(QJsonObject::fromVariantMap(m_config));
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR("ConfigManager", QString("无法写入配置文件: %1").arg(filePath));
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        LOG_ERROR("ConfigManager", QString("无法写入配置文件: %1").arg(filePath));
        return false;
    }
    m_configFilePath = filePath;
    LOG_INFO("ConfigManager", QString("配置已保存: %1").arg(filePath));
    return true;
}
QVariant ConfigManager::value(const QString& key, const QVariant& defaultValue) const {
    return m_config.value(key, defaultValue);
}
void ConfigManager::setValue(const QString& key, const QVariant& value) {
    m_config[key] = value;
    emit configChanged(key);
}
int ConfigManager::maxParallelTasks() const {
    return m_config.value("maxParallelTasks", 4).toInt();
}
void ConfigManager::setMaxParallelTasks(int count) {
    setValue("maxParallelTasks", count);
}
QString ConfigManager::outputDirectory() const {
    return m_config.value("outputDirectory", QDir::homePath()).toString();
}
void ConfigManager::setOutputDirectory(const QString& dir) {
    setValue("outputDirectory", dir);
}
int ConfigManager::logLevel() const {
    return m_config.value("logLevel", 1).toInt();
}
void ConfigManager::setLogLevel(int level) {
    setValue("logLevel", level);
}
