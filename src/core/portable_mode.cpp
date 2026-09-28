#include "portable_mode.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

namespace PortableMode
{

bool isPortable(const QString& appDirPath)
{
    return QFileInfo::exists(QDir(appDirPath).filePath(QStringLiteral("portable.flag")));
}

QString dataDir(const QString& appDirPath, const QString& standardDir)
{
    return isPortable(appDirPath) ? QDir(appDirPath).absolutePath() : standardDir;
}

QString currentDataDir()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    if (isPortable(appDir))
    {
        return QDir(appDir).absolutePath();
    }
    // Lazy: keep <QStandardPaths> off the header include chain.
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}

} // namespace PortableMode
