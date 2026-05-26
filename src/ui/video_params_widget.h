#ifndef VIDEO_PARAMS_WIDGET_H
#define VIDEO_PARAMS_WIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QSlider>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QVariantMap>

class VideoParamsWidget : public QWidget {
    Q_OBJECT
public:
    explicit VideoParamsWidget(QWidget* parent = nullptr);
    ~VideoParamsWidget() override = default;

    QVariantMap getParams() const;
    void setParams(const QVariantMap& params);
    QStringList validate() const;

signals:
    void paramsChanged();

private:
    void setupUI();
    void setupConnections();
    QString buildPreviewText() const;

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
    QLabel* m_previewLabel;
};

#endif // VIDEO_PARAMS_WIDGET_H
