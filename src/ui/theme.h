#pragma once
// Central design system — all colors, spacing and widget chrome live here.
// Borrowed from FFmpegFreeUI v6 (MIT): semantic action colors, gray-block +
// left-bar selection, three-layer surface hierarchy, dark/light parity.
#include <QColor>
#include <QString>

class QApplication;
class QWidget;

namespace Theme
{

enum class Mode
{
    System,
    Light,
    Dark
};

struct Palette
{
    QColor bg, surface, surfaceAlt, sidebar;
    QColor text, textSecondary, textMuted;
    QColor border, divider;
    QColor accent, accentSoft, accentText;
    QColor success, successSoft;
    QColor warning, warningSoft;
    QColor danger, dangerSoft;
    QColor remove, removeSoft;
    QColor locate, locateSoft;
    bool dark = false;
};

const Palette& palette(Mode mode);
Mode resolve(Mode requested); // System -> Light|Dark via QStyleHints

Mode currentMode(); // what was last applied
const Palette& current();

// Build the full application QSS for the given mode.
QString styleSheet(Mode mode);

// Apply app-wide stylesheet + font; remembers the resolved mode.
void apply(QApplication* app, Mode mode);

// Tag a widget with a semantic class consumed by the QSS, e.g.
//   setCss(btn, "primary") / "ghost-success" / "chip-accent" / "muted"
// Re-polishes the widget so theme switches pick it up.
void setCss(QWidget* w, const char* cls);

Mode loadMode();          // from ConfigManager ("ui.theme": system|light|dark)
void saveMode(Mode mode); // persists and re-applies to the running app

} // namespace Theme
