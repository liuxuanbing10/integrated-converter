#include "test_large_file_handler.h"
#include "core/large_file_handler.h"
#include <QTest>
#include <QTemporaryFile>

void TestLargeFileHandler::testCategorizeBySize() {
    QCOMPARE(LargeFileHandler::categorizeBySize(0), LargeFileHandler::FileSizeCategory::Small);
    QCOMPARE(LargeFileHandler::categorizeBySize(50LL * 1024 * 1024 - 1), LargeFileHandler::FileSizeCategory::Small);
    QCOMPARE(LargeFileHandler::categorizeBySize(120LL * 1024 * 1024), LargeFileHandler::FileSizeCategory::Medium);
    QCOMPARE(LargeFileHandler::categorizeBySize(600LL * 1024 * 1024), LargeFileHandler::FileSizeCategory::Large);
    QCOMPARE(LargeFileHandler::categorizeBySize(2LL * 1024 * 1024 * 1024), LargeFileHandler::FileSizeCategory::VeryLarge);
    QCOMPARE(LargeFileHandler::categorizeBySize(10LL * 1024 * 1024 * 1024), LargeFileHandler::FileSizeCategory::Huge);
}

void TestLargeFileHandler::testCategorizeBySizeBoundaries() {
    QCOMPARE(LargeFileHandler::categorizeBySize(LargeFileHandler::MEDIUM_THRESHOLD), LargeFileHandler::FileSizeCategory::Medium);
    QCOMPARE(LargeFileHandler::categorizeBySize(LargeFileHandler::LARGE_THRESHOLD), LargeFileHandler::FileSizeCategory::Large);
    QCOMPARE(LargeFileHandler::categorizeBySize(LargeFileHandler::VERY_LARGE_THRESHOLD), LargeFileHandler::FileSizeCategory::VeryLarge);
    QCOMPARE(LargeFileHandler::categorizeBySize(LargeFileHandler::HUGE_THRESHOLD), LargeFileHandler::FileSizeCategory::Huge);
}

void TestLargeFileHandler::testFormatFileSize() {
    QCOMPARE(LargeFileHandler::formatFileSize(0), QString("0 B"));
    QCOMPARE(LargeFileHandler::formatFileSize(1023), QString("1023 B"));
    QVERIFY(LargeFileHandler::formatFileSize(1024).contains("KB"));
    QVERIFY(LargeFileHandler::formatFileSize(1024 * 1024).contains("MB"));
    QVERIFY(LargeFileHandler::formatFileSize(1024LL * 1024 * 1024).contains("GB"));
    QVERIFY(LargeFileHandler::formatFileSize(5LL * 1024 * 1024 * 1024).contains("GB"));
}

void TestLargeFileHandler::testFormatFileSizeNegative() {
    QCOMPARE(LargeFileHandler::formatFileSize(-1), QString::fromUtf8("\xe6\x9c\xaa\xe7\x9f\xa5"));
}

void TestLargeFileHandler::testGetFileSizeNonExistent() {
    QCOMPARE(LargeFileHandler::getFileSize("/nonexistent/path/file.mp4"), 0);
}

void TestLargeFileHandler::testShouldUseStreamCopy() {
    QVERIFY(LargeFileHandler::shouldUseStreamCopy("mp4", "mp4"));
    QVERIFY(LargeFileHandler::shouldUseStreamCopy("mp4", "mov"));
    QVERIFY(LargeFileHandler::shouldUseStreamCopy("avi", "mkv"));
    QVERIFY(LargeFileHandler::shouldUseStreamCopy("mkv", "avi"));
}

void TestLargeFileHandler::testShouldUseStreamCopyMismatch() {
    QVERIFY(!LargeFileHandler::shouldUseStreamCopy("mp4", "avi"));
    QVERIFY(!LargeFileHandler::shouldUseStreamCopy("mp3", "wav"));
}

void TestLargeFileHandler::testRecommendedPriority() {
    QCOMPARE(LargeFileHandler::recommendedPriority(LargeFileHandler::FileSizeCategory::Small), 2);
    QCOMPARE(LargeFileHandler::recommendedPriority(LargeFileHandler::FileSizeCategory::Medium), 1);
    QCOMPARE(LargeFileHandler::recommendedPriority(LargeFileHandler::FileSizeCategory::Large), 0);
}
