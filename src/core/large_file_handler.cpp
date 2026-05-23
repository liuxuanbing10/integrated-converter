#include "large_file_handler.h"
#include "memory_monitor.h"
#include <QFileInfo>

bool LargeFileHandler::isLargeFile(const QString& filePath, qint64 threshold) {
    return getFileSize(filePath) >= threshold;
}

LargeFileHandler::FileSizeCategory LargeFileHandler::categorizeFile(const QString& filePath) {
    return categorizeBySize(getFileSize(filePath));
}

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
    if (bytes < 1024) {
        return QString("%1 B").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    } else if (bytes < 1024LL * 1024LL * 1024LL) {
        return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    } else if (bytes < 1024LL * 1024LL * 1024LL * 1024LL) {
        return QString("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    } else {
        return QString("%1 TB").arg(bytes / (1024.0 * 1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    }
}

QVariantMap LargeFileHandler::optimizeParams(const QVariantMap& params, qint64 fileSize) {
    QVariantMap optimized = params;
    FileSizeCategory category = categorizeBySize(fileSize);
    if (category >= FileSizeCategory::Large) {
        if (!optimized.contains("preset") || optimized.value("preset").toString() == "slow") {
            optimized["preset"] = "fast";
        }
        if (category >= FileSizeCategory::VeryLarge) {
            optimized["preset"] = "veryfast";
        }
    }
    if (category >= FileSizeCategory::VeryLarge) {
        int currentBitrate = optimized.value("videoBitrate", 0).toInt();
        if (currentBitrate > 8000) {
            optimized["videoBitrate"] = 8000;
        }
    }
    if (category >= FileSizeCategory::Huge) {
        int width = optimized.value("width", 0).toInt();
        int height = optimized.value("height", 0).toInt();
        if (width == 0 || height == 0) {
            optimized["resolution"] = "1280x720";
        } else if (width > 1920 || height > 1080) {
            double scale = 1920.0 / qMax(width, height);
            int newWidth = static_cast<int>(width * scale);
            int newHeight = static_cast<int>(height * scale);
            newWidth = (newWidth / 2) * 2;
            newHeight = (newHeight / 2) * 2;
            optimized["resolution"] = QString("%1x%2").arg(newWidth).arg(newHeight);
        }
    }
    return optimized;
}

QVariantMap LargeFileHandler::getSuggestions(const QString& filePath) {
    QVariantMap suggestions;
    qint64 size = getFileSize(filePath);
    FileSizeCategory category = categorizeBySize(size);
    suggestions["category"] = categoryToString(category);
    suggestions["fileSize"] = size;
    suggestions["fileSizeFormatted"] = formatFileSize(size);
    suggestions["isLarge"] = category >= FileSizeCategory::Medium;
    suggestions["needsSegmented"] = shouldUseSegmentedConversion(filePath);
    if (category >= FileSizeCategory::Medium) {
        suggestions["suggestedPreset"] = "fast";
    }
    if (category >= FileSizeCategory::Large) {
        suggestions["suggestedPreset"] = "veryfast";
        suggestions["suggestedResolution"] = "1280x720";
    }
    if (category >= FileSizeCategory::VeryLarge) {
        suggestions["suggestedSegmentCount"] = recommendedSegmentCount(filePath);
        suggestions["warning"] = QObject::tr("文件非常大，建议使用分段转换以避免内存问题");
    }
    if (category >= FileSizeCategory::Huge) {
        suggestions["suggestedPreset"] = "ultrafast";
        suggestions["suggestedResolution"] = "854x480";
        suggestions["warning"] = QObject::tr("超大文件，强烈建议降低分辨率和使用快速编码预设");
    }
    suggestions["recommendedPriority"] = recommendedPriority(category);
    return suggestions;
}

bool LargeFileHandler::shouldUseSegmentedConversion(const QString& filePath) {
    qint64 size = getFileSize(filePath);
    return size >= VERY_LARGE_THRESHOLD;
}

int LargeFileHandler::recommendedSegmentCount(const QString& filePath) {
    qint64 size = getFileSize(filePath);
    if (size < VERY_LARGE_THRESHOLD) {
        return 1;
    }
    int segments = static_cast<int>(size / (500LL * 1024LL * 1024LL));
    return qBound(2, segments, 10);
}

QString LargeFileHandler::categoryToString(FileSizeCategory category) {
    switch (category) {
        case FileSizeCategory::Small:     return QObject::tr("小文件");
        case FileSizeCategory::Medium:    return QObject::tr("中等文件");
        case FileSizeCategory::Large:     return QObject::tr("大文件");
        case FileSizeCategory::VeryLarge: return QObject::tr("超大文件");
        case FileSizeCategory::Huge:      return QObject::tr("巨型文件");
        default:                          return QObject::tr("未知");
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
