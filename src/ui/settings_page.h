#ifndef SETTINGS_PAGE_H
#define SETTINGS_PAGE_H

#include <QWidget>

namespace Theme
{
enum class Mode;
}

/// §零.2 assembly extraction: the settings page (appearance / performance /
/// notifications) built once by MainWindow::setupCentralWidget. Controls
/// write straight through ConfigManager/TaskManager exactly as the inline
/// version did; only the theme switch needs MainWindow-side polish, so it
/// travels as a signal.
class SettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPage(QWidget* parent = nullptr);

signals:
    void themeModeRequested(Theme::Mode mode);
};

#endif // SETTINGS_PAGE_H
