#ifndef IMAGE_PARAMS_WIDGET_H
#define IMAGE_PARAMS_WIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QSlider>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>
#include <QVariantMap>
#include <QRegularExpressionValidator>

class ImageParamsWidget : public QWidget {
    Q_OBJECT
public:
    explicit ImageParamsWidget(QWidget* parent = nullptr);
    ~ImageParamsWidget() override = default;

    QVariantMap getParams() const;
    void setParams(const QVariantMap& params);
    void setEnabledFormats(const QStringList& formats);
    QStringList validate() const;

signals:
    void paramsChanged();

private:
    void setupUI();
    void setupConnections();
    QString buildPreviewText() const;

    QSlider* m_qualitySlider;
    QSpinBox* m_qualitySpinBox;
    QLineEdit* m_resizeInput;
    QComboBox* m_compressionCombo;
    QSpinBox* m_densitySpinBox;
    QCheckBox* m_stripCheckBox;
    QComboBox* m_depthCombo;
    QLabel* m_previewLabel;
};

#endif // IMAGE_PARAMS_WIDGET_H
