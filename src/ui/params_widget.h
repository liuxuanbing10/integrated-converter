#ifndef PARAMS_WIDGET_H
#define PARAMS_WIDGET_H

#include <QList>
#include <QPair>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QLabel;
class QSpinBox;
class QVBoxLayout;
QT_END_NAMESPACE

/// Shared skeleton for the four category param widgets (analysis §6.5).
/// Owns what used to be copy-pasted ~120x per widget:
///  - the "参数预览" QGroupBox + monospace preview label (Theme class
///    "preview"; no inline QSS anywhere below this point)
///  - bulk blockSignals guarding around setParams via the widget registry
///  - one-line signal wiring from every registered control to the preview
///    refresh + paramsChanged emission
///  - slider<->spinbox pair sync
///  - small factory helpers for the two most common controls
/// Subclasses declare their option rows (combo/spin/text) inside group
/// boxes, override collectParams/applyParams and buildPreviewText.
class AbstractParamsWidget : public QWidget
{
    Q_OBJECT
public:
    /// (displayText, userData) pair used by addCombo.
    using Choice = QPair<QString, QVariant>;

    explicit AbstractParamsWidget(QWidget* parent = nullptr);
    ~AbstractParamsWidget() override = default;

    QVariantMap getParams() const;
    void setParams(const QVariantMap& params);
    virtual QStringList validate() const = 0;

signals:
    void paramsChanged();

protected:
    /// Main vertical layout: subclass sections go before the preview box.
    QVBoxLayout* m_body;

    /// Register a control whose change signal drives the preview refresh.
    /// `sig` must be the SIGNAL() text of a no-arg-compatible value-changed
    /// signal, e.g. SIGNAL("currentIndexChanged(int)").
    void registerControl(QWidget* w, const char* sig);

    /// Add a titled QGroupBox to the body and return its grid layout
    /// (columns: label | control; pass spans to occupy both).
    class QGridLayout* addGroup(const QString& title);

    /// Labelled combo added to `grid` at (row,0)/(row,1); registered +
    /// pre-filled from `choices`; returns the combo.
    QComboBox* addCombo(QGridLayout* grid, int row, const QString& label, const QList<Choice>& choices,
                        int minHeight = 30);

    /// Labelled spin (int or double depending on `spinIsDouble`) added to
    /// `grid` at (row,col), spanning `colSpan`; registered; returns widget
    /// cast for the caller to keep the concrete pointer.
    QWidget* addSpin(QGridLayout* grid, int row, int col, int colSpan, const QString& label, bool spinIsDouble,
                     QWidget* spin);

    /// A muted hint QLabel spanning the full grid width under `row`.
    void addHint(QGridLayout* grid, int row, const QString& text);

    /// Keep a slider and a spinbox in lockstep (shared min/max/range must be
    /// set by the caller beforehand).
    void syncPair(class QSlider* slider, QSpinBox* spin);

    /// Finalize: creates the preview group + label and starts the refresh.
    void finishSetup();

    /// Rebuild the preview text from the current control values.
    virtual QString buildPreviewText() const = 0;
    /// Read control values into the params map (base: fresh map).
    virtual QVariantMap collectParams() const = 0;
    /// Push saved params into the controls (inside a signal-blocked batch).
    virtual void applyParams(const QVariantMap& params) = 0;
    /// Optional extra reaction to any registered control changing
    /// (e.g. enable/disable dependents). Runs after the preview refresh.
    virtual void onControlChanged()
    { }

    static void setComboData(QComboBox* combo, const QVariant& data);
    static QString separatorLine();

    /// Rebuild the preview text without emitting paramsChanged (used when an
    /// external setter only needs to sync the label).
    void repaintPreview();

    QList<QPair<QWidget*, const char*>> m_controls;

private slots:
    /// Shared sink for every registered control's change signal
    /// (string/SIGNAL() wiring: Qt's classic generic connect path).
    void onParamControlChanged();

private:
    void refreshPreview();

    QGroupBox* m_previewGroup = nullptr;
    QLabel* m_previewLabel = nullptr;
};

#endif // PARAMS_WIDGET_H
