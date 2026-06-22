#ifndef LARGE_FILE_HANDLER_H
#define LARGE_FILE_HANDLER_H

#include <QString>

class LargeFileHandler {
public:
    enum class FileSizeCategory {
        Small,
        Medium,
        Large,
        VeryLarge,
        Huge
    };

    static constexpr qint64 MEDIUM_THRESHOLD = 100 * 1024 * 1024;
    static constexpr qint64 LARGE_THRESHOLD = 500 * 1024 * 1024;
    static constexpr qint64 VERY_LARGE_THRESHOLD = 1024LL * 1024LL * 1024LL;
    static constexpr qint64 HUGE_THRESHOLD = 5LL * 1024LL * 1024LL * 1024LL;

    static FileSizeCategory categorizeBySize(qint64 size);
    static qint64 getFileSize(const QString& filePath);
    static QString formatFileSize(qint64 bytes);
    static int recommendedPriority(FileSizeCategory category);
    static bool shouldUseStreamCopy(const QString& inputFormat, const QString& outputFormat);
};
#endif // LARGE_FILE_HANDLER_H
