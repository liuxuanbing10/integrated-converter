#ifndef TEST_LARGE_FILE_HANDLER_H
#define TEST_LARGE_FILE_HANDLER_H

#include <QObject>
#include <QString>

class TestLargeFileHandler : public QObject {
    Q_OBJECT
public:
    explicit TestLargeFileHandler(QObject* parent = nullptr) : QObject(parent) {}
private slots:
    void testCategorizeBySize();
    void testCategorizeBySizeBoundaries();
    void testFormatFileSize();
    void testFormatFileSizeNegative();
    void testGetFileSizeNonExistent();
    void testIsLargeFile();
    void testIsLargeFileBelowThreshold();
    void testOptimizeParamsSmallFile();
    void testOptimizeParamsLargeFile();
    void testOptimizeParamsHugeFile();
    void testShouldUseStreamCopy();
    void testShouldUseStreamCopyMismatch();
    void testCategoryToString();
    void testRecommendedPriority();
    void testShouldUseSegmentedConversion();
    void testRecommendedSegmentCount();
};

#endif // TEST_LARGE_FILE_HANDLER_H
