#include "window_drop.h"

#include <QDir>
#include <QDropEvent>
#include <QFileInfo>
#include <QMimeData>
#include <QUrl>

#include <algorithm>

namespace WindowDrop
{

bool hasLocalFileUrls(const QMimeData* mimeData)
{
    return mimeData && mimeData->hasUrls() &&
           std::any_of(mimeData->urls().cbegin(), mimeData->urls().cend(),
                       [](const QUrl& u) { return u.isLocalFile(); });
}

QStringList pathsFromDrop(QDropEvent* event)
{
    QStringList paths;
    if (!event->mimeData()->hasUrls())
    {
        return paths;
    }
    for (const QUrl& url : event->mimeData()->urls())
    {
        if (!url.isLocalFile())
        {
            continue;
        }
        const QString localPath = url.toLocalFile();
        QFileInfo info(localPath);
        if (info.isDir())
        {
            QDir dir(localPath);
            for (const QString& name : dir.entryList(QDir::Files))
            {
                paths << dir.absoluteFilePath(name);
            }
        }
        else if (info.isFile())
        {
            paths << localPath;
        }
    }
    return paths;
}

} // namespace WindowDrop
