#include "params_widget.h"

#include "theme.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

AbstractParamsWidget::AbstractParamsWidget(QWidget* parent) : QWidget(parent), m_body(nullptr)
{
    m_body = new QVBoxLayout(this);
    m_body->setContentsMargins(16, 16, 16, 16);
    m_body->setSpacing(12);
}

void AbstractParamsWidget::registerControl(QWidget* w, const char* sig)
{
    m_controls.append(qMakePair(w, sig));
}

QGridLayout* AbstractParamsWidget::addGroup(const QString& title)
{
    QGroupBox* group = new QGroupBox(title);
    QGridLayout* grid = new QGridLayout(group);
    grid->setSpacing(8);
    m_body->addWidget(group);
    return grid;
}

QComboBox* AbstractParamsWidget::addCombo(QGridLayout* grid, int row, const QString& label,
                                          const QList<Choice>& choices, int minHeight)
{
    grid->addWidget(new QLabel(label), row, 0);
    QComboBox* combo = new QComboBox();
    for (const auto& c : choices)
    {
        combo->addItem(c.first, c.second);
    }
    combo->setMinimumHeight(minHeight);
    grid->addWidget(combo, row, 1);
    registerControl(combo, SIGNAL(currentIndexChanged(int)));
    return combo;
}

QWidget* AbstractParamsWidget::addSpin(QGridLayout* grid, int row, int col, int colSpan, const QString& label,
                                       bool spinIsDouble, QWidget* spin)
{
    if (!label.isEmpty())
    {
        grid->addWidget(new QLabel(label), row, col);
        col += 1;
        colSpan -= 1;
    }
    grid->addWidget(spin, row, col, 1, qMax(1, colSpan));
    registerControl(spin, spinIsDouble ? SIGNAL(valueChanged(double)) : SIGNAL(valueChanged(int)));
    return spin;
}

void AbstractParamsWidget::addHint(QGridLayout* grid, int row, const QString& text)
{
    QLabel* hint = new QLabel(text);
    Theme::setCss(hint, "hint");
    grid->addWidget(hint, row, 0, 1, 4);
}

void AbstractParamsWidget::syncPair(QSlider* slider, QSpinBox* spin)
{
    connect(slider, &QSlider::valueChanged, spin, &QSpinBox::setValue);
    connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), slider, &QSlider::setValue);
}

void AbstractParamsWidget::finishSetup()
{
    // Preview panel — was duplicated verbatim in all four widgets (§6.5).
    m_previewGroup = new QGroupBox(tr("参数预览"));
    QVBoxLayout* previewLayout = new QVBoxLayout(m_previewGroup);
    m_previewLabel = new QLabel();
    m_previewLabel->setWordWrap(true);
    m_previewLabel->setMinimumHeight(200);
    Theme::setCss(m_previewLabel, "preview");
    previewLayout->addWidget(m_previewLabel);
    m_body->addWidget(m_previewGroup);
    m_body->addStretch();

    for (const auto& c : std::as_const(m_controls))
    {
        connect(c.first, c.second, this, SLOT(onParamControlChanged()));
    }
    refreshPreview();
}

void AbstractParamsWidget::onParamControlChanged()
{
    refreshPreview();
    onControlChanged();
}

void AbstractParamsWidget::refreshPreview()
{
    m_previewLabel->setText(buildPreviewText());
    emit paramsChanged();
}

void AbstractParamsWidget::repaintPreview()
{
    m_previewLabel->setText(buildPreviewText());
}

QVariantMap AbstractParamsWidget::getParams() const
{
    return collectParams();
}

void AbstractParamsWidget::setParams(const QVariantMap& params)
{
    // One guard for *every* registered control: adding a widget no longer
    // means remembering blockSignals(true)/(false) twice by hand.
    for (const auto& c : m_controls)
    {
        c.first->blockSignals(true);
    }
    applyParams(params);
    for (const auto& c : m_controls)
    {
        c.first->blockSignals(false);
    }
    m_previewLabel->setText(buildPreviewText());
}

void AbstractParamsWidget::setComboData(QComboBox* combo, const QVariant& data)
{
    const int idx = combo->findData(data);
    if (idx >= 0)
        combo->setCurrentIndex(idx);
}

QString AbstractParamsWidget::separatorLine()
{
    return QStringLiteral("━━━━━━━━━━━━━━━━━━\n");
}
