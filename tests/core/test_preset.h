#ifndef TEST_PRESET_H
#define TEST_PRESET_H

#include <QObject>

/// §一.1 acceptance: 往返测试（存→删→载→params 全等）+ schema 迁移守卫 +
/// 分享文件格式契约（拒绝非 preset 文件）。
class TestPreset : public QObject
{
    Q_OBJECT
public:
    explicit TestPreset(QObject* parent = nullptr) : QObject(parent)
    { }

private slots:
    void testJsonRoundTripFullEquality();
    void testLibrarySaveLoadRemoveRoundTrip();
    void testValidateRejectsBadPresets();
    void testShareMarkerRejectsForeignJson();
    void testShareFileExportImportRoundTrip();
    void testSchemaVersionSurvivesUnknownFields();
};

#endif // TEST_PRESET_H
