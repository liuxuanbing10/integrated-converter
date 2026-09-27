#ifndef VIDEO_PARAMS_WIDGET_H
#define VIDEO_PARAMS_WIDGET_H

#include "params_widget.h"

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QLineEdit;
class QSlider;
class QSpinBox;
QT_END_NAMESPACE

class VideoParamsWidget : public AbstractParamsWidget
{
    Q_OBJECT
public:
    explicit VideoParamsWidget(QWidget* parent = nullptr);
    ~VideoParamsWidget() override = default;

    QStringList validate() const override;

protected:
    QVariantMap collectParams() const override;
    void applyParams(const QVariantMap& params) override;
    QString buildPreviewText() const override;

private:
    QComboBox* m_videoCodecCombo;
    QComboBox* m_audioCodecCombo;
    QLineEdit* m_resolutionInput;
    QComboBox* m_videoBitrateCombo;
    QComboBox* m_audioBitrateCombo;
    QComboBox* m_framerateCombo;
    QComboBox* m_presetCombo;
    QSlider* m_crfSlider;
    QSpinBox* m_crfSpinBox;
    QCheckBox* m_twoPassCheckBox;
};

#endif // VIDEO_PARAMS_WIDGET_H
