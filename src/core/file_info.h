#ifndef FILE_INFO_H
#define FILE_INFO_H

#include <QString>

/// Metadata for one queued input file shared across the UI category tabs.
/// Lives in core (not in a widget header) so UI modules don't have to
/// include an unused widget just to name the struct.
struct FileInfo
{
    QString filePath;
    QString fileName;
    qint64 fileSize;
    QString format;
    bool selected;
    FileInfo() : fileSize(0), selected(false)
    { }
    FileInfo(const QString& path, const QString& name, qint64 size, const QString& fmt) :
        filePath(path), fileName(name), fileSize(size), format(fmt), selected(false)
    { }
};

#endif // FILE_INFO_H
