#ifndef AUDIO_PARAMS_WIDGET_H
#define AUDIO_PARAMS_WIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QSlider>
#include <QSpinBox>
#include <QLabel>
#include <QVariantMap>

class AudioParamsWidget : public QWidget {
    Q_OBJECT
public:
    explicit AudioParamsWidget(QWidget* parent = nullptr);
    ~AudioParamsWidget() override = default;

    QVariantMap getParams() const;
    void setParams(const QVariantMap& params);
    QStringList validate() const;

signals:
    void paramsChanged();

private:
    void setupUI();
    void setupConnections();
    QString buildPreviewText() const;

    QComboBox* m_bitrateCombo;
    QComboBox* m_sampleRateCombo;
    QComboBox* m_channelsCombo;
    QSlider* m_vbrQualitySlider;
    QSpinBox* m_vbrQualitySpinBox;
    QComboBox* m_codecCombo;
    QLabel* m_previewLabel;
};

#endif // AUDIO_PARAMS_WIDGET_H
