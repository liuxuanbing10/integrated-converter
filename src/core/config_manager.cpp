#include "config_manager.h"

#include "logger.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSaveFile>
ConfigManager& ConfigManager::instance()
{
    static ConfigManager instance;
    return instance;
}
ConfigManager::ConfigManager()
{
    // §7.1: the constructor must be trivial. Tool detection used to run four
    // findExecutable() scans here (candidate list x `--version` process
    // probes) on the MAIN THREAD before the window appears. Detection is now
    // lazy (ensureToolPaths), memoized, and skipped when a stored path is
    // already usable — steady-state startup does four QFileInfo::exists.
    initDefaultConfig();
}
ConfigManager::~ConfigManager()
{ }
void ConfigManager::initDefaultConfig()
{
    m_config[QStringLiteral("maxParallelTasks")] = 4;
    m_config[QStringLiteral("outputDirectory")] = QDir::homePath();
    m_config[QStringLiteral("logLevel")] = 1;
    m_config[QStringLiteral("autoStartConversion")] = false;
    m_config[QStringLiteral("overwriteExisting")] = false;
    m_config[QStringLiteral("showNotification")] = true;
    m_config[QStringLiteral("ffmpegPath")] = QStringLiteral("ffmpeg");
    m_config[QStringLiteral("pandocPath")] = QStringLiteral("pandoc");
    m_config[QStringLiteral("imagemagickPath")] = QStringLiteral("magick");
}
QString ConfigManager::findExecutable(const QString& name)
{
    QStringList possiblePaths;
    possiblePaths << name;
    possiblePaths << QDir::homePath() + "/bin/" + name;
    possiblePaths << QDir::rootPath() + "Program Files/ffmpeg/bin/" + name + ".exe";
    possiblePaths << QDir::rootPath() + "Program Files (x86)/ffmpeg/bin/" + name + ".exe";
    possiblePaths << QCoreApplication::applicationDirPath() + "/" + name;
    possiblePaths << QCoreApplication::applicationDirPath() + "/tools/" + name;
    // ponytail: scan common package manager dirs (WinGet, scoop) for ffmpeg/pandoc/magick
    QStringList pkgRoots = {QDir::homePath() + "/AppData/Local/Microsoft/WinGet/Packages",
                            QDir::homePath() + "/scoop/apps"};
    for (const QString& root : pkgRoots)
    {
        QDir rootDir(root);
        if (!rootDir.exists())
            continue;
        for (const QString& pkg : rootDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        {
            // ponytail: skip shared builds — DLLs can't be found by QProcess, causes crash
            if (pkg.contains(QStringLiteral("Shared"), Qt::CaseInsensitive))
                continue;
            QString directBin = root + "/" + pkg + "/bin/" + name + ".exe";
            if (QFileInfo::exists(directBin))
            {
                possiblePaths << directBin;
                continue;
            }
            QDir pkgDir(root + "/" + pkg);
            for (const QString& sub : pkgDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
            {
                QString deepBin = root + "/" + pkg + "/" + sub + "/bin/" + name + ".exe";
                if (QFileInfo::exists(deepBin))
                {
                    possiblePaths << deepBin;
                }
            }
        }
    }
    for (const QString& path : possiblePaths)
    {
        QFileInfo fi(path);
        if (fi.exists() && fi.isExecutable())
        {
            return path;
        }
        // 用 --version 验证可执行文件，等待完成再销毁 QProcess，避免 "Destroyed while process running" 警告
        QProcess p;
        p.start(path, QStringList() << "--version");
        if (p.waitForStarted(3000) && p.waitForFinished(5000))
        {
            return path;
        }
    }
    return name;
}
bool ConfigManager::toolPathUsable(const QString& key, const QString& exeName) const
{
    const QString val = m_config.value(key).toString();
    if (val.isEmpty() || val.compare(exeName, Qt::CaseInsensitive) == 0)
    {
        return false; // missing or bare PATH-name: cannot trust, re-verify
    }
    if (val.contains(QStringLiteral("Shared"), Qt::CaseInsensitive))
    {
        return false; // stale shared build: DLLs unloadable by QProcess
    }
    return QFileInfo::exists(val) || QFileInfo::exists(val + QStringLiteral(".exe"));
}

void ConfigManager::detectTool(const QString& key, const QString& exeName)
{
    if (toolPathUsable(key, exeName))
    {
        return; // stored absolute path still on disk — zero probing
    }
    const QString detected = findExecutable(exeName);
    if (detected != exeName)
    {
        m_config[key] = detected;
        LOG_INFO("ConfigManager", QString("自动检测到%1: %2").arg(key, detected));
    }
    else
    {
        // Nothing resolvable: normalize to the honest bare PATH-name so a
        // stale absolute path never reaches QProcess (which would fail with
        // a confusing launch error instead of "not installed").
        m_config[key] = exeName;
    }
}

namespace
{
/// True iff `exe --version` runs AND its combined output mentions `needle`.
/// Guards the IM6 fallback: Windows ships a SYSTEM convert.exe (FAT->NTFS)
/// that also starts and exits on --version — without this check CI resolved
/// "ImageMagick" to the disk-conversion utility.
bool probeOutputContains(const QString& exe, const QString& needle)
{
    QProcess p;
    p.start(exe, QStringList() << QStringLiteral("--version"));
    if (!p.waitForStarted(3000) || !p.waitForFinished(5000))
    {
        return false;
    }
    const QString out =
        QString::fromLocal8Bit(p.readAllStandardOutput()) + QString::fromLocal8Bit(p.readAllStandardError());
    return out.contains(needle, Qt::CaseInsensitive);
}
} // namespace

void ConfigManager::ensureToolPaths()
{
    if (m_toolsResolved)
    {
        return; // one resolve per session
    }
    m_toolsResolved = true;
    detectFFmpegPath();
    detectFFprobePath();
    detectPandocPath();
    detectImageMagickPath();
}

void ConfigManager::detectFFmpegPath()
{
    detectTool(QStringLiteral("ffmpegPath"), QStringLiteral("ffmpeg"));
}
void ConfigManager::detectFFprobePath()
{
    detectTool(QStringLiteral("ffprobePath"), QStringLiteral("ffprobe"));
}
void ConfigManager::detectPandocPath()
{
    detectTool(QStringLiteral("pandocPath"), QStringLiteral("pandoc"));
}
void ConfigManager::detectImageMagickPath()
{
    if (toolPathUsable(QStringLiteral("imagemagickPath"), QStringLiteral("magick")))
    {
        return;
    }
    // ImageMagick 7+ uses 'magick', IM6 uses 'convert'.
    QString detected = findExecutable(QStringLiteral("magick"));
    QString flavor = QStringLiteral("magick");
    if (detected == QStringLiteral("magick"))
    {
        // magick unavailable — IM6 fallback, but ONLY if the candidate
        // actually identifies itself as ImageMagick (see probe note).
        detected = findExecutable(QStringLiteral("convert"));
        flavor = QStringLiteral("convert");
        if (detected == QStringLiteral("convert") || !probeOutputContains(detected, QStringLiteral("ImageMagick")))
        {
            detected = QStringLiteral("magick"); // nothing trustworthy found
            flavor.clear();
        }
    }
    if (!flavor.isEmpty())
    {
        m_config[QStringLiteral("imagemagickPath")] = detected;
        LOG_INFO("ConfigManager", QString("自动检测到ImageMagick(%1)路径: %2").arg(flavor, detected));
    }
    else
    {
        m_config[QStringLiteral("imagemagickPath")] = QStringLiteral("magick"); // honest fallback
    }
}
bool ConfigManager::loadConfig(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        LOG_WARNING("ConfigManager", QString("无法打开配置文件: %1").arg(filePath));
        return false;
    }
    QByteArray data = file.readAll();
    file.close();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError)
    {
        LOG_ERROR("ConfigManager", QString("配置文件解析错误: %1").arg(error.errorString()));
        return false;
    }
    if (!doc.isObject())
    {
        LOG_ERROR("ConfigManager", "配置文件格式错误: 根元素不是对象");
        return false;
    }
    m_config = doc.object().toVariantMap();
    m_configFilePath = filePath;
    // loadConfig() replaces m_config wholesale, possibly with bare names or
    // stale Shared paths. Re-run resolution over the file's values (the
    // per-tool usable-guard skips every still-valid stored path for free).
    m_toolsResolved = false;
    ensureToolPaths();
    LOG_INFO("ConfigManager", QString("配置已加载: %1").arg(filePath));
    return true;
}
bool ConfigManager::saveConfig(const QString& filePath)
{
    QJsonDocument doc(QJsonObject::fromVariantMap(m_config));
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        LOG_ERROR("ConfigManager", QString("无法写入配置文件: %1").arg(filePath));
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    if (!file.commit())
    {
        LOG_ERROR("ConfigManager", QString("无法写入配置文件: %1").arg(filePath));
        return false;
    }
    m_configFilePath = filePath;
    LOG_INFO("ConfigManager", QString("配置已保存: %1").arg(filePath));
    return true;
}
QVariant ConfigManager::value(const QString& key, const QVariant& defaultValue) const
{
    return m_config.value(key, defaultValue);
}
void ConfigManager::setValue(const QString& key, const QVariant& value)
{
    m_config[key] = value;
}
int ConfigManager::maxParallelTasks() const
{
    return m_config.value(QStringLiteral("maxParallelTasks"), 4).toInt();
}
void ConfigManager::setMaxParallelTasks(int count)
{
    setValue(QStringLiteral("maxParallelTasks"), count);
}
QString ConfigManager::outputDirectory() const
{
    return m_config.value(QStringLiteral("outputDirectory"), QDir::homePath()).toString();
}
void ConfigManager::setOutputDirectory(const QString& dir)
{
    setValue(QStringLiteral("outputDirectory"), dir);
}
int ConfigManager::logLevel() const
{
    return m_config.value(QStringLiteral("logLevel"), 1).toInt();
}
void ConfigManager::setLogLevel(int level)
{
    setValue(QStringLiteral("logLevel"), level);
}
