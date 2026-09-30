#ifndef CONVERSION_PARAMS_DIALOG_H
#define CONVERSION_PARAMS_DIALOG_H

#include "format_registry.h"

#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVariantMap>

class ImageParamsWidget;
class DocumentParamsWidget;
class AudioParamsWidget;
class VideoParamsWidget;

class ConversionParamsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ConversionParamsDialog(QWidget* parent = nullptr);
    ~ConversionParamsDialog() override = default;

    /// Set the active category — switches to the appropriate params page
    void setActiveCategory(FormatRegistry::Category category);
    FormatRegistry::Category activeCategory() const
    {
        return m_activeCategory;
    }

    /// Get merged params for all categories (or for the active category)
    QVariantMap getParams() const;
    QVariantMap getParamsForCategory(FormatRegistry::Category category) const;

    /// Set params for a specific category
    void setParamsForCategory(FormatRegistry::Category category, const QVariantMap& params);

    /// Load saved preferences from ConfigManager
    void loadPreferences();
    /// Save current preferences to ConfigManager
    void savePreferences();

    /// Enable dark mode styling
    void setDarkMode(bool enabled);

signals:
    void paramsAccepted(FormatRegistry::Category category, const QVariantMap& params);
    void paramsChanged(FormatRegistry::Category category);
    /// §一.1 preset save request: shell persists (category, params, name).
    void saveAsPresetRequested(FormatRegistry::Category category, const QVariantMap& params, const QString& name);

private slots:
    void onCategoryChanged(int index);
    void onValidateAndAccept();
    void onResetToDefaults();
    void onAnyParamsChanged();
    /// §一.1: capture current-category params as a named preset (the shell
    /// owns the library + target format, so this travels as a signal).
    void onSaveAsPreset();

private:
    void setupUI();
    void setupConnections();
    void applyDialogStyleSheet();

    FormatRegistry::Category m_activeCategory;

    // Category selector
    QComboBox* m_categoryCombo;

    // Parameter pages
    QStackedWidget* m_stackedWidget;
    ImageParamsWidget* m_imageParams;
    DocumentParamsWidget* m_docParams;
    AudioParamsWidget* m_audioParams;
    VideoParamsWidget* m_videoParams;

    // Per-category saved state
    QMap<FormatRegistry::Category, QVariantMap> m_savedParams;

    // UI controls
    QPushButton* m_okBtn;
    QPushButton* m_cancelBtn;
    QPushButton* m_resetBtn;
    QPushButton* m_presetBtn;
    QLabel* m_statusLabel;
    bool m_darkMode;
};

#endif // CONVERSION_PARAMS_DIALOG_H
