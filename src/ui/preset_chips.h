#ifndef PRESET_CHIPS_H
#define PRESET_CHIPS_H

#include "preset.h"

#include <QHash>
#include <QWidget>

class QHBoxLayout;

/// §一.1 GUI surface: one chip row beside the format combo showing the
/// presets that match the ACTIVE category. Click = apply (shell writes the
/// result into combo + per-category params); right-click = 导出分享文件 /
/// 删除. Pure view over PresetLibrary — no conversion semantics here.
class PresetChipsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PresetChipsWidget(PresetLibrary* library, QWidget* parent = nullptr);

    /// Rebuild chips for `category` (empty = hide the row entirely).
    void showCategory(FormatRegistry::Category category);

signals:
    void applyRequested(const Preset& preset);

private:
    void rebuild();
    void exportPreset(const Preset& preset);
    void deletePreset(const Preset& preset);

    PresetLibrary* m_library;
    QHBoxLayout* m_layout;
    FormatRegistry::Category m_category = FormatRegistry::Category::Unknown;
};

#endif // PRESET_CHIPS_H
