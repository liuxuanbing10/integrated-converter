#include "large_file_handler.h"
#include <QFileInfo>

constexpr qint64 KB = 1024;
constexpr qint64 MB = KB * 1024;
constexpr qint64 GB = MB * 1024;
constexpr qint64 TB = GB * 1024;

LargeFileHandler::FileSizeCategory LargeFileHandler::categorizeBySize(qint64 size) {
    if (size >= HUGE_THRESHOLD) {
        return FileSizeCategory::Huge;
    } else if (size >= VERY_LARGE_THRESHOLD) {
        return FileSizeCategory::VeryLarge;
    } else if (size >= LARGE_THRESHOLD) {
        return FileSizeCategory::Large;
    } else if (size >= MEDIUM_THRESHOLD) {
        return FileSizeCategory::Medium;
    }
    return FileSizeCategory::Small;
}

qint64 LargeFileHandler::getFileSize(const QString& filePath) {
    QFileInfo info(filePath);
    if (info.exists()) {
        return info.size();
    }
    return 0;
}

QString LargeFileHandler::formatFileSize(qint64 bytes) {
    if (bytes < 0) {
        return QString("未知");
    }
    if (bytes < KB) {
        return QString("%1 B").arg(bytes);
    } else if (bytes < MB) {
        return QString("%1 KB").arg(bytes / static_cast<double>(KB), 0, 'f', 1);
    } else if (bytes < GB) {
        return QString("%1 MB").arg(bytes / static_cast<double>(MB), 0, 'f', 1);
    } else if (bytes < TB) {
        return QString("%1 GB").arg(bytes / static_cast<double>(GB), 0, 'f', 2);
    } else {
        return QString("%1 TB").arg(bytes / static_cast<double>(TB), 0, 'f', 2);
    }
}

int LargeFileHandler::recommendedPriority(FileSizeCategory category) {
    switch (category) {
        case FileSizeCategory::Small:     return 2;
        case FileSizeCategory::Medium:    return 1;
        case FileSizeCategory::Large:     return 0;
        case FileSizeCategory::VeryLarge: return 0;
        case FileSizeCategory::Huge:      return 0;
        default:                          return 1;
    }
}

bool LargeFileHandler::shouldUseStreamCopy(const QString& inputFormat, const QString& outputFormat) {
    QString input = inputFormat.toLower();
    QString output = outputFormat.toLower();
    if (input == output) {
        return true;
    }
    if ((input == "mp4" && output == "mov") ||
        (input == "mov" && output == "mp4")) {
        return true;
    }
    if ((input == "avi" && output == "mkv") ||
        (input == "mkv" && output == "avi")) {
        return true;
    }
    return false;
}
