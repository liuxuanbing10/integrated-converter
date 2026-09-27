#ifndef AUDIO_PARAMS_WIDGET_H
#define AUDIO_PARAMS_WIDGET_H

#include "params_widget.h"

QT_BEGIN_NAMESPACE
class QComboBox;
class QSlider;
class QSpinBox;
QT_END_NAMESPACE

class AudioParamsWidget : public AbstractParamsWidget
{
    Q_OBJECT
public:
    explicit AudioParamsWidget(QWidget* parent = nullptr);
    ~AudioParamsWidget() override = default;

    QStringList validate() const override;

protected:
    QVariantMap collectParams() const override;
    void applyParams(const QVariantMap& params) override;
    QString buildPreviewText() const override;

private:
    QComboBox* m_codecCombo;
    QComboBox* m_bitrateCombo;
    QComboBox* m_sampleRateCombo;
    QComboBox* m_channelsCombo;
    QSlider* m_vbrQualitySlider;
    QSpinBox* m_vbrQualitySpinBox;
};

#endif // AUDIO_PARAMS_WIDGET_H
