#ifndef IMAGE_PARAMS_WIDGET_H
#define IMAGE_PARAMS_WIDGET_H

#include "params_widget.h"

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QLineEdit;
class QSlider;
class QSpinBox;
QT_END_NAMESPACE

class ImageParamsWidget : public AbstractParamsWidget
{
    Q_OBJECT
public:
    explicit ImageParamsWidget(QWidget* parent = nullptr);
    ~ImageParamsWidget() override = default;

    void setEnabledFormats(const QStringList& formats);
    QStringList validate() const override;

protected:
    QVariantMap collectParams() const override;
    void applyParams(const QVariantMap& params) override;
    QString buildPreviewText() const override;

private:
    QSlider* m_qualitySlider;
    QSpinBox* m_qualitySpinBox;
    QLineEdit* m_resizeInput;
    QComboBox* m_compressionCombo;
    QSpinBox* m_densitySpinBox;
    QCheckBox* m_stripCheckBox;
    QComboBox* m_depthCombo;
};

#endif // IMAGE_PARAMS_WIDGET_H
