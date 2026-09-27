#ifndef DOCUMENT_PARAMS_WIDGET_H
#define DOCUMENT_PARAMS_WIDGET_H

#include "params_widget.h"

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
QT_END_NAMESPACE

class DocumentParamsWidget : public AbstractParamsWidget
{
    Q_OBJECT
public:
    explicit DocumentParamsWidget(QWidget* parent = nullptr);
    ~DocumentParamsWidget() override = default;

    QStringList validate() const override;

protected:
    QVariantMap collectParams() const override;
    void applyParams(const QVariantMap& params) override;
    void onControlChanged() override;
    QString buildPreviewText() const override;

private:
    QComboBox* m_pageSizeCombo;
    QComboBox* m_orientationCombo;
    QDoubleSpinBox* m_marginTop;
    QDoubleSpinBox* m_marginBottom;
    QDoubleSpinBox* m_marginLeft;
    QDoubleSpinBox* m_marginRight;
    QComboBox* m_pdfEngineCombo;
    QCheckBox* m_tocCheckBox;
    QSpinBox* m_tocDepthSpinBox;
    QCheckBox* m_numberSectionsCheckBox;
};

#endif // DOCUMENT_PARAMS_WIDGET_H
