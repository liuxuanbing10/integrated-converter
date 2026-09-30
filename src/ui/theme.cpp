#include "theme.h"

#include "core/config_manager.h"

#include <QApplication>
#include <QStyleHints>
#include <QWidget>

namespace Theme
{

namespace
{

Palette makeLight()
{
    Palette p;
    p.bg = QColor(0xf2, 0xf4, 0xf7);         // window canvas
    p.surface = QColor(0xff, 0xff, 0xff);    // cards / panels
    p.surfaceAlt = QColor(0xf7, 0xf9, 0xfb); // headers, zebra rows
    p.sidebar = QColor(0xe9, 0xec, 0xf1);
    p.text = QColor(0x1d, 0x21, 0x29);
    p.textSecondary = QColor(0x4e, 0x59, 0x69);
    p.textMuted = QColor(0x86, 0x90, 0x9c);
    p.border = QColor(0xdc, 0xe1, 0xe8);
    p.divider = QColor(0xe8, 0xeb, 0xf0);
    p.accent = QColor(0x16, 0x64, 0xff);
    p.accentSoft = QColor(0xeb, 0xf1, 0xff);
    p.accentText = QColor(0xff, 0xff, 0xff);
    p.success = QColor(0x2a, 0x81, 0x4b);
    p.successSoft = QColor(0xe2, 0xf5, 0xeb);
    p.warning = QColor(0xbd, 0x7e, 0x00);
    p.warningSoft = QColor(0xfb, 0xf3, 0xde);
    p.danger = QColor(0xd7, 0x31, 0x2a);
    p.dangerSoft = QColor(0xfe, 0xec, 0xed);
    p.remove = QColor(0xe0, 0x5f, 0x2f);
    p.removeSoft = QColor(0xfd, 0xef, 0xe7);
    p.locate = QColor(0x7b, 0x5c, 0xf0);
    p.locateSoft = QColor(0xf0, 0xec, 0xfe);
    p.dark = false;
    return p;
}

Palette makeDark()
{
    // Same semantic roles, 3FUI-dark style: near-blue-black layers,
    // desaturated text, accents lifted for contrast on dark masks.
    Palette p;
    p.bg = QColor(0x14, 0x17, 0x1d);
    p.surface = QColor(0x1d, 0x21, 0x29);
    p.surfaceAlt = QColor(0x23, 0x28, 0x32);
    p.sidebar = QColor(0x19, 0x1c, 0x23);
    p.text = QColor(0xe6, 0xe8, 0xec);
    p.textSecondary = QColor(0xb0, 0xb7, 0xc1);
    p.textMuted = QColor(0x7e, 0x86, 0x92);
    p.border = QColor(0x35, 0x3c, 0x48);
    p.divider = QColor(0x2b, 0x31, 0x3b);
    p.accent = QColor(0x5a, 0x9c, 0xf0);
    p.accentSoft = QColor(0x24, 0x34, 0x4e);
    p.accentText = QColor(0x0d, 0x12, 0x1c);
    p.success = QColor(0x5f, 0xd0, 0x68);
    p.successSoft = QColor(0x1e, 0x35, 0x26);
    p.warning = QColor(0xf2, 0xc1, 0x4e);
    p.warningSoft = QColor(0x3a, 0x30, 0x1b);
    p.danger = QColor(0xf0, 0x50, 0x50);
    p.dangerSoft = QColor(0x3c, 0x21, 0x24);
    p.remove = QColor(0xf0, 0x8a, 0x50);
    p.removeSoft = QColor(0x3a, 0x28, 0x1e);
    p.locate = QColor(0x9d, 0x7b, 0xff);
    p.locateSoft = QColor(0x2c, 0x26, 0x45);
    p.dark = true;
    return p;
}

} // namespace

const Palette& palette(Mode mode)
{
    static const Palette light = makeLight();
    static const Palette dark = makeDark();
    return resolve(mode) == Mode::Dark ? dark : light;
}

Mode resolve(Mode requested)
{
    if (requested != Mode::System)
        return requested;
    const auto scheme = QApplication::styleHints()->colorScheme();
    return scheme == Qt::ColorScheme::Dark ? Mode::Dark : Mode::Light;
}

static Mode g_currentMode = Mode::Light;

Mode currentMode()
{
    return g_currentMode;
}

const Palette& current()
{
    return palette(g_currentMode);
}

QString styleSheet(Mode mode)
{
    const Palette& p = palette(mode);
    QString qss = QStringLiteral(R"(
/* ── base ─────────────────────────────────────────────────────── */
QWidget { background-color: {bg}; color: {text}; font-size: 13px; }
QToolTip { background-color: {surface}; color: {text};
           border: 1px solid {border}; padding: 4px 8px; }

/* ── sidebar / navigation ─────────────────────────────────────── */
#sidebar { background-color: {sidebar}; border: none; }
QLabel[cssClass="nav-section"] { color: {textMuted}; font-size: 11px; font-weight: 700;
    padding: 10px 14px 2px 14px; background: transparent; letter-spacing: 1px; }
#sidebarSearch QLineEdit { background-color: {surface}; border: 1px solid {border};
    border-radius: 8px; padding: 6px 10px; }
QPushButton[cssClass="nav"] { background: transparent; border: none; border-left: 3px solid transparent;
    padding: 9px 14px; text-align: left; font-size: 13px; font-weight: 600;
    color: {textSecondary}; border-radius: 0px; }
QPushButton[cssClass="nav"]:hover { background-color: {divider}; color: {text}; }
QPushButton[cssClass="nav"][selected="true"] { background-color: {surfaceAlt};
    border-left: 3px solid {accent}; color: {accent}; }

/* ── panels / groups ──────────────────────────────────────────── */
QFrame[cssClass="panel"] { background-color: {surface}; border: 1px solid {border}; border-radius: 12px; }
QFrame[cssClass="panel"] QLabel { background: transparent; }
QGroupBox { background-color: {surface}; border: 1px solid {border}; border-radius: 12px;
    margin-top: 10px; padding-top: 8px; font-weight: 600; }
QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; color: {text}; }
QFrame[cssClass="divider"] { border: none; border-top: 1px solid {divider}; background: transparent; }

/* ── tabs ─────────────────────────────────────────────────────── */
QTabWidget::pane { border: 1px solid {border}; border-radius: 12px;
    background-color: {surface}; }
QTabBar { background: transparent; }
QTabBar::tab { background: transparent; border: none; border-bottom: 2px solid transparent;
    padding: 9px 18px; font-size: 13px; font-weight: 600; color: {textSecondary}; }
QTabBar::tab:hover:!selected { color: {text}; background-color: {surfaceAlt};
    border-top-left-radius: 8px; border-top-right-radius: 8px; }
QTabBar::tab:selected { color: {accent}; border-bottom: 2px solid {accent}; }

/* ── buttons: semantic text actions (3FUI style) ──────────────── */
QPushButton { background-color: {surface}; color: {text}; border: 1px solid {border};
    border-radius: 8px; padding: 6px 14px; }
QPushButton:hover { background-color: {surfaceAlt}; }
QPushButton:pressed { background-color: {divider}; }
QPushButton:disabled { color: {textMuted}; border-color: {divider}; }

QPushButton[cssClass="primary"] { background-color: {accent}; color: {accentText};
    border: none; font-weight: 600; }
QPushButton[cssClass="primary"]:hover { background-color: {accent}; }
QPushButton[cssClass="primary"]:pressed { border: 1px solid {accent}; }
QPushButton[cssClass="primary"]:disabled { background-color: {divider}; color: {textMuted}; }

QPushButton[cssClass="accent-outline"] { background-color: {accentSoft}; color: {accent};
    border: 1px solid {accent}; font-weight: 600; }
QPushButton[cssClass="success-outline"] { background-color: {successSoft}; color: {success};
    border: 1px solid {success}; font-weight: 600; }

QPushButton[cssClass="text"] { background: transparent; border: none; border-radius: 8px;
    padding: 6px 14px; font-weight: 600; }
QPushButton[cssClass="text-success"] { color: {success}; }
QPushButton[cssClass="text-warning"] { color: {warning}; }
QPushButton[cssClass="text-danger"]  { color: {danger}; }
QPushButton[cssClass="text-remove"]  { color: {remove}; }
QPushButton[cssClass="text-locate"]  { color: {locate}; }
QPushButton[cssClass="text-accent"]  { color: {accent}; }
QPushButton[cssClass^="text"]:hover { background-color: {divider}; }
QPushButton[cssClass="text"]:disabled { color: {textMuted}; }

/* row-level mini buttons: tighter */
QPushButton[cssClass="row-danger"] { background: transparent; border: none; color: {danger};
    font-size: 12px; padding: 2px 8px; border-radius: 6px; }
QPushButton[cssClass="row-danger"]:hover { background-color: {dangerSoft}; }

/* ── inputs ───────────────────────────────────────────────────── */
QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox { background-color: {surface};
    border: 1px solid {border}; border-radius: 8px; padding: 4px 8px; color: {text}; }
QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus { border: 1px solid {accent}; }
QLineEdit:disabled { background-color: {surfaceAlt}; color: {textMuted}; }
QLineEdit[cssClass="placeholder-hint"] { color: {textMuted}; }
QLabel[cssClass="hint"] { color: {textMuted}; font-size: 11px; background: transparent; }
QLabel[cssClass="preview"] { background-color: {surfaceAlt}; border: 1px solid {border};
    border-radius: 8px; padding: 12px; font-family: 'Consolas', 'Courier New', monospace;
    font-size: 12px; color: {text}; }
QComboBox::drop-down { border: none; width: 22px; }
QComboBox QAbstractItemView { background-color: {surface}; border: 1px solid {border};
    border-radius: 8px; selection-background-color: {accentSoft}; selection-color: {accent};
    outline: none; }

/* ── tables (queue view) ──────────────────────────────────────── */
QTableWidget { background-color: {surface}; border: 1px solid {border}; border-radius: 12px;
    gridline-color: {divider}; alternate-background-color: {surfaceAlt}; }
QTableWidget::item { padding: 4px 6px; border: none; }
QTableWidget::item:selected { background-color: {accentSoft}; color: {accent}; }
QHeaderView::section { background-color: {surfaceAlt}; border: none;
    border-right: 1px solid {divider}; border-bottom: 1px solid {border};
    padding: 7px 6px; font-weight: 600; color: {textSecondary}; }
QTableCornerButton::section { background-color: {surfaceAlt}; border: none; }

/* ── progress ─────────────────────────────────────────────────── */
QProgressBar { background-color: {surfaceAlt}; border: 1px solid {border};
    border-radius: 5px; text-align: center; color: {textSecondary}; font-size: 11px; }
QProgressBar::chunk { background-color: {accent}; border-radius: 4px; }
QProgressBar[cssClass="bar-success"]::chunk { background-color: {success}; }
QProgressBar[cssClass="bar-warning"]::chunk { background-color: {warning}; }

/* ── chips (stat callouts) ────────────────────────────────────── */
QLabel[cssClass="chip"] { background-color: {surfaceAlt}; color: {textSecondary};
    border-radius: 6px; padding: 3px 10px; font-weight: 600; }
QLabel[cssClass="chip-accent"] { background-color: {accentSoft}; color: {accent};
    border-radius: 6px; padding: 3px 10px; font-weight: 600; }
QLabel[cssClass="chip-success"] { background-color: {successSoft}; color: {success};
    border-radius: 6px; padding: 3px 10px; font-weight: 600; }
QLabel[cssClass="chip-warning"] { background-color: {warningSoft}; color: {warning};
    border-radius: 6px; padding: 3px 10px; font-weight: 600; }
QLabel[cssClass="chip-danger"] { background-color: {dangerSoft}; color: {danger};
    border-radius: 6px; padding: 3px 10px; font-weight: 600; }
/* §一.1 preset chips: clickable chip buttons beside the format combo */
QPushButton[cssClass="chip-btn"] { background-color: {accentSoft}; color: {accent};
    border: 1px solid {accentSoft}; border-radius: 10px; padding: 3px 12px; font-size: 11px; font-weight: 600; }
QPushButton[cssClass="chip-btn"]:hover { border-color: {accent}; }
QPushButton[cssClass="chip-btn"]:pressed { background-color: {surfaceAlt}; }

/* ── text roles ───────────────────────────────────────────────── */
QLabel[cssClass="title"] { font-size: 15px; font-weight: 700; color: {text}; background: transparent; }
QLabel[cssClass="section"] { font-size: 13px; font-weight: 700; color: {accent}; background: transparent; }
QLabel[cssClass="muted"] { color: {textMuted}; font-size: 12px; background: transparent; }
QLabel[cssClass="hint-ok"] { color: {success}; font-size: 12px; background: transparent; }
QLabel[cssClass="hint-warn"] { color: {warning}; font-size: 12px; background: transparent; }
QLabel[cssClass="stat"] { font-size: 20px; font-weight: 700; background: transparent; }

/* ── scrollbars ───────────────────────────────────────────────── */
QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
QScrollBar::handle:vertical { background-color: {border}; border-radius: 4px; min-height: 30px; }
QScrollBar::handle:vertical:hover { background-color: {textMuted}; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
QScrollBar:horizontal { background: transparent; height: 10px; margin: 2px; }
QScrollBar::handle:horizontal { background-color: {border}; border-radius: 4px; min-width: 30px; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }

/* ── misc ─────────────────────────────────────────────────────── */
QMenu { background-color: {surface}; border: 1px solid {border}; border-radius: 8px; padding: 4px; }
QMenu::item { padding: 6px 22px; border-radius: 6px; }
QMenu::item:selected { background-color: {accentSoft}; color: {accent}; }
QMenu::separator { height: 1px; background: {divider}; margin: 4px 8px; }
QStatusBar { background-color: {surface}; color: {textSecondary}; border-top: 1px solid {border}; }
QSplitter::handle { background-color: transparent; }
QCheckBox, QRadioButton { background: transparent; spacing: 6px; }
QListWidget, QTreeWidget, QPlainTextEdit, QTextEdit { background-color: {surface};
    border: 1px solid {border}; border-radius: 8px; color: {text}; }
QCheckBox::indicator, QRadioButton::indicator { width: 16px; height: 16px; }
QScrollArea { border: none; background: transparent; }
)")
                      .replace("{bg}", p.bg.name())
                      .replace("{surface}", p.surface.name())
                      .replace("{surfaceAlt}", p.surfaceAlt.name())
                      .replace("{sidebar}", p.sidebar.name())
                      .replace("{text}", p.text.name())
                      .replace("{textSecondary}", p.textSecondary.name())
                      .replace("{textMuted}", p.textMuted.name())
                      .replace("{border}", p.border.name())
                      .replace("{divider}", p.divider.name())
                      .replace("{accent}", p.accent.name())
                      .replace("{accentSoft}", p.accentSoft.name())
                      .replace("{accentText}", p.accentText.name())
                      .replace("{success}", p.success.name())
                      .replace("{successSoft}", p.successSoft.name())
                      .replace("{warning}", p.warning.name())
                      .replace("{warningSoft}", p.warningSoft.name())
                      .replace("{danger}", p.danger.name())
                      .replace("{dangerSoft}", p.dangerSoft.name())
                      .replace("{remove}", p.remove.name())
                      .replace("{removeSoft}", p.removeSoft.name())
                      .replace("{locate}", p.locate.name())
                      .replace("{locateSoft}", p.locateSoft.name());
    return qss;
}

void setCss(QWidget* w, const char* cls)
{
    w->setProperty("cssClass", QLatin1String(cls));
}

void apply(QApplication* app, Mode mode)
{
    g_currentMode = resolve(mode);
    app->setStyleSheet(styleSheet(g_currentMode));
}

Mode loadMode()
{
    const QString v = ConfigManager::instance().value(QStringLiteral("ui.theme"), QStringLiteral("system")).toString();
    if (v == QLatin1String("light"))
        return Mode::Light;
    if (v == QLatin1String("dark"))
        return Mode::Dark;
    return Mode::System;
}

void saveMode(Mode mode)
{
    const char* v = mode == Mode::Light ? "light" : (mode == Mode::Dark ? "dark" : "system");
    ConfigManager::instance().setValue(QStringLiteral("ui.theme"), QString::fromLatin1(v));
    if (auto* app = qobject_cast<QApplication*>(QApplication::instance()))
    {
        apply(app, mode);
    }
}

} // namespace Theme
