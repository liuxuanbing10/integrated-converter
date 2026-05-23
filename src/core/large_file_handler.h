#ifndef LARGE_FILE_HANDLER_H
#define LARGE_FILE_HANDLER_H

#include <QString>
#include <QVariantMap>
#include <QFileInfo>

class LargeFileHandler {
public:
    enum class FileSizeCategory {
        Small,
        Medium,
        Large,
        VeryLarge,
        Huge
    };

    static constexpr qint64 SMALL_THRESHOLD = 50 * 1024 * 1024;
    static constexpr qint64 MEDIUM_THRESHOLD = 100 * 1024 * 1024;
    static constexpr qint64 LARGE_THRESHOLD = 500 * 1024 * 1024;
    static constexpr qint64 VERY_LARGE_THRESHOLD = 1024LL * 1024LL * 1024LL;
    static constexpr qint64 HUGE_THRESHOLD = 5LL * 1024LL * 1024LL * 1024LL;

    static bool isLargeFile(const QString& filePath, qint64 threshold = MEDIUM_THRESHOLD);
    static FileSizeCategory categorizeFile(const QString& filePath);
    static FileSizeCategory categorizeBySize(qint64 size);
    static qint64 getFileSize(const QString& filePath);
    static QString formatFileSize(qint64 bytes);
    static QVariantMap optimizeParams(const QVariantMap& params, qint64 fileSize);
    static QVariantMap getSuggestions(const QString& filePath);
    static bool shouldUseSegmentedConversion(const QString& filePath);
    static int recommendedSegmentCount(const QString& filePath);
    static QString categoryToString(FileSizeCategory category);
    static int recommendedPriority(FileSizeCategory category);
    static bool shouldUseStreamCopy(const QString& inputFormat, const QString& outputFormat);
};
#endif // LARGE_FILE_HANDLER_H
