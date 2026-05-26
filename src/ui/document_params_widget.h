#ifndef DOCUMENT_PARAMS_WIDGET_H
#define DOCUMENT_PARAMS_WIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QVariantMap>

class DocumentParamsWidget : public QWidget {
    Q_OBJECT
public:
    explicit DocumentParamsWidget(QWidget* parent = nullptr);
    ~DocumentParamsWidget() override = default;

    QVariantMap getParams() const;
    void setParams(const QVariantMap& params);
    QStringList validate() const;

signals:
    void paramsChanged();

private:
    void setupUI();
    void setupConnections();
    QString buildPreviewText() const;

    QComboBox* m_pageSizeCombo;
    QComboBox* m_orientationCombo;
    QDoubleSpinBox* m_marginTop;
    QDoubleSpinBox* m_marginBottom;
    QDoubleSpinBox* m_marginLeft;
    QDoubleSpinBox* m_marginRight;
    QComboBox* m_pdfEngineCombo;
    QCheckBox* m_tocCheckBox;
    QCheckBox* m_numberSectionsCheckBox;
    QSpinBox* m_tocDepthSpinBox;
    QLabel* m_previewLabel;
};

#endif // DOCUMENT_PARAMS_WIDGET_H
