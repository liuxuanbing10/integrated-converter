#include "document_params_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFrame>

DocumentParamsWidget::DocumentParamsWidget(QWidget* parent)
    : QWidget(parent)
    , m_pageSizeCombo(nullptr)
    , m_orientationCombo(nullptr)
    , m_marginTop(nullptr)
    , m_marginBottom(nullptr)
    , m_marginLeft(nullptr)
    , m_marginRight(nullptr)
    , m_pdfEngineCombo(nullptr)
    , m_tocCheckBox(nullptr)
    , m_numberSectionsCheckBox(nullptr)
    , m_tocDepthSpinBox(nullptr)
    , m_previewLabel(nullptr)
{
    setupUI();
    setupConnections();
}

void DocumentParamsWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // ── Page layout ───────────────────────────────────────────────
    QGroupBox* pageGroup = new QGroupBox(tr("页面布局"));
    QGridLayout* pageGrid = new QGridLayout(pageGroup);
    pageGrid->setSpacing(8);

    pageGrid->addWidget(new QLabel(tr("页面大小:")), 0, 0);
    m_pageSizeCombo = new QComboBox();
    m_pageSizeCombo->addItem("A4 (210×297mm)", "a4");
    m_pageSizeCombo->addItem("A5 (148×210mm)", "a5");
    m_pageSizeCombo->addItem("Letter (216×279mm)", "letter");
    m_pageSizeCombo->addItem("Legal (216×356mm)", "legal");
    m_pageSizeCombo->addItem("Tabloid (279×432mm)", "tabloid");
    m_pageSizeCombo->setMinimumHeight(30);
    pageGrid->addWidget(m_pageSizeCombo, 0, 1);

    pageGrid->addWidget(new QLabel(tr("方向:")), 1, 0);
    m_orientationCombo = new QComboBox();
    m_orientationCombo->addItem(tr("纵向"), "portrait");
    m_orientationCombo->addItem(tr("横向"), "landscape");
    m_orientationCombo->setMinimumHeight(30);
    pageGrid->addWidget(m_orientationCombo, 1, 1);

    mainLayout->addWidget(pageGroup);

    // ── Margins ───────────────────────────────────────────────────
    QGroupBox* marginGroup = new QGroupBox(tr("页边距 (英寸)"));
    QGridLayout* marginGrid = new QGridLayout(marginGroup);
    marginGrid->setSpacing(8);

    auto createMarginSpinBox = [this]() -> QDoubleSpinBox* {
        QDoubleSpinBox* sp = new QDoubleSpinBox();
        sp->setRange(0.0, 5.0);
        sp->setSingleStep(0.1);
        sp->setValue(1.0);
        sp->setSuffix(tr(" 英寸"));
        sp->setDecimals(1);
        sp->setMinimumHeight(30);
        sp->setStyleSheet(
            "QDoubleSpinBox { padding: 4px; border: 1px solid #ccc; border-radius: 6px; font-size: 13px; }"
        );
        return sp;
    };

    marginGrid->addWidget(new QLabel(tr("上:")), 0, 0);
    m_marginTop = createMarginSpinBox();
    marginGrid->addWidget(m_marginTop, 0, 1);

    marginGrid->addWidget(new QLabel(tr("下:")), 0, 2);
    m_marginBottom = createMarginSpinBox();
    marginGrid->addWidget(m_marginBottom, 0, 3);

    marginGrid->addWidget(new QLabel(tr("左:")), 1, 0);
    m_marginLeft = createMarginSpinBox();
    marginGrid->addWidget(m_marginLeft, 1, 1);

    marginGrid->addWidget(new QLabel(tr("右:")), 1, 2);
    m_marginRight = createMarginSpinBox();
    marginGrid->addWidget(m_marginRight, 1, 3);

    mainLayout->addWidget(marginGroup);

    // ── PDF options ───────────────────────────────────────────────
    QGroupBox* pdfGroup = new QGroupBox(tr("PDF/输出选项"));
    QGridLayout* pdfGrid = new QGridLayout(pdfGroup);
    pdfGrid->setSpacing(8);

    pdfGrid->addWidget(new QLabel(tr("PDF引擎:")), 0, 0);
    m_pdfEngineCombo = new QComboBox();
    m_pdfEngineCombo->addItem(tr("默认"), "");
    m_pdfEngineCombo->addItem("pdflatex", "pdflatex");
    m_pdfEngineCombo->addItem("xelatex", "xelatex");
    m_pdfEngineCombo->addItem("lualatex", "lualatex");
    m_pdfEngineCombo->addItem("wkhtmltopdf", "wkhtmltopdf");
    m_pdfEngineCombo->addItem("weasyprint", "weasyprint");
    m_pdfEngineCombo->setMinimumHeight(30);
    pdfGrid->addWidget(m_pdfEngineCombo, 0, 1);

    m_tocCheckBox = new QCheckBox(tr("生成目录 (Table of Contents)"));
    m_tocCheckBox->setChecked(false);
    pdfGrid->addWidget(m_tocCheckBox, 1, 0, 1, 2);

    QHBoxLayout* tocDepthLayout = new QHBoxLayout();
    tocDepthLayout->addWidget(new QLabel(tr("目录深度:")), 0);
    m_tocDepthSpinBox = new QSpinBox();
    m_tocDepthSpinBox->setRange(1, 6);
    m_tocDepthSpinBox->setValue(3);
    m_tocDepthSpinBox->setMinimumHeight(30);
    m_tocDepthSpinBox->setEnabled(false);
    tocDepthLayout->addWidget(m_tocDepthSpinBox);
    tocDepthLayout->addStretch();
    pdfGrid->addLayout(tocDepthLayout, 2, 0, 1, 2);

    m_numberSectionsCheckBox = new QCheckBox(tr("章节编号 (Number Sections)"));
    m_numberSectionsCheckBox->setChecked(false);
    pdfGrid->addWidget(m_numberSectionsCheckBox, 3, 0, 1, 2);

    mainLayout->addWidget(pdfGroup);

    // ── Preview ───────────────────────────────────────────────────
    QGroupBox* previewGroup = new QGroupBox(tr("参数预览"));
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);
    m_previewLabel = new QLabel();
    m_previewLabel->setWordWrap(true);
    m_previewLabel->setStyleSheet(
        "QLabel { background-color: #f5f5f5; border: 1px solid #e0e0e0; "
        "border-radius: 6px; padding: 12px; font-family: 'Consolas', 'Courier New', monospace; "
        "font-size: 12px; color: #333; }"
    );
    m_previewLabel->setMinimumHeight(200);
    previewLayout->addWidget(m_previewLabel);
    mainLayout->addWidget(previewGroup);

    mainLayout->addStretch();
}

void DocumentParamsWidget::setupConnections() {
    auto updatePreview = [this]() {
        m_previewLabel->setText(buildPreviewText());
        emit paramsChanged();
    };

    connect(m_pageSizeCombo, &QComboBox::currentTextChanged, this, updatePreview);
    connect(m_orientationCombo, &QComboBox::currentTextChanged, this, updatePreview);
    connect(m_marginTop, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, updatePreview);
    connect(m_marginBottom, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, updatePreview);
    connect(m_marginLeft, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, updatePreview);
    connect(m_marginRight, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, updatePreview);
    connect(m_pdfEngineCombo, &QComboBox::currentTextChanged, this, updatePreview);
    connect(m_tocCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        m_tocDepthSpinBox->setEnabled(checked);
        m_previewLabel->setText(buildPreviewText());
        emit paramsChanged();
    });
    connect(m_tocDepthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, updatePreview);
    connect(m_numberSectionsCheckBox, &QCheckBox::toggled, this, updatePreview);
}

QVariantMap DocumentParamsWidget::getParams() const {
    QVariantMap params;
    params["pageSize"] = m_pageSizeCombo->currentData().toString();
    params["orientation"] = m_orientationCombo->currentData().toString();
    params["marginTop"] = m_marginTop->value();
    params["marginBottom"] = m_marginBottom->value();
    params["marginLeft"] = m_marginLeft->value();
    params["marginRight"] = m_marginRight->value();
    params["pdfEngine"] = m_pdfEngineCombo->currentData().toString();
    params["toc"] = m_tocCheckBox->isChecked();
    params["tocDepth"] = m_tocDepthSpinBox->value();
    params["numberSections"] = m_numberSectionsCheckBox->isChecked();
    return params;
}

void DocumentParamsWidget::setParams(const QVariantMap& params) {
    m_pageSizeCombo->blockSignals(true);
    m_orientationCombo->blockSignals(true);
    m_marginTop->blockSignals(true);
    m_marginBottom->blockSignals(true);
    m_marginLeft->blockSignals(true);
    m_marginRight->blockSignals(true);
    m_pdfEngineCombo->blockSignals(true);
    m_tocCheckBox->blockSignals(true);
    m_tocDepthSpinBox->blockSignals(true);
    m_numberSectionsCheckBox->blockSignals(true);

    auto setComboData = [](QComboBox* combo, const QVariant& data) {
        int idx = combo->findData(data);
        if (idx >= 0) combo->setCurrentIndex(idx);
    };

    setComboData(m_pageSizeCombo, params.value("pageSize", "a4"));
    setComboData(m_orientationCombo, params.value("orientation", "portrait"));
    m_marginTop->setValue(params.value("marginTop", 1.0).toDouble());
    m_marginBottom->setValue(params.value("marginBottom", 1.0).toDouble());
    m_marginLeft->setValue(params.value("marginLeft", 1.0).toDouble());
    m_marginRight->setValue(params.value("marginRight", 1.0).toDouble());
    setComboData(m_pdfEngineCombo, params.value("pdfEngine", ""));
    m_tocCheckBox->setChecked(params.value("toc", false).toBool());
    m_tocDepthSpinBox->setValue(params.value("tocDepth", 3).toInt());
    m_tocDepthSpinBox->setEnabled(m_tocCheckBox->isChecked());
    m_numberSectionsCheckBox->setChecked(params.value("numberSections", false).toBool());

    m_pageSizeCombo->blockSignals(false);
    m_orientationCombo->blockSignals(false);
    m_marginTop->blockSignals(false);
    m_marginBottom->blockSignals(false);
    m_marginLeft->blockSignals(false);
    m_marginRight->blockSignals(false);
    m_pdfEngineCombo->blockSignals(false);
    m_tocCheckBox->blockSignals(false);
    m_tocDepthSpinBox->blockSignals(false);
    m_numberSectionsCheckBox->blockSignals(false);

    m_previewLabel->setText(buildPreviewText());
}

QStringList DocumentParamsWidget::validate() const {
    QStringList errors;
    if (m_marginTop->value() < 0 || m_marginBottom->value() < 0 ||
        m_marginLeft->value() < 0 || m_marginRight->value() < 0) {
        errors << tr("页边距不能为负数");
    }
    return errors;
}

QString DocumentParamsWidget::buildPreviewText() const {
    QString text = tr("📋 文档参数预览\n");
    text += tr("━━━━━━━━━━━━━━━━━━\n");
    text += tr("页面大小: %1\n").arg(m_pageSizeCombo->currentText());
    text += tr("方向: %1\n").arg(m_orientationCombo->currentText());
    text += tr("边距: 上%1 / 下%2 / 左%3 / 右%4\n")
        .arg(QString::number(m_marginTop->value(), 'f', 1))
        .arg(QString::number(m_marginBottom->value(), 'f', 1))
        .arg(QString::number(m_marginLeft->value(), 'f', 1))
        .arg(QString::number(m_marginRight->value(), 'f', 1));
    if (!m_pdfEngineCombo->currentData().toString().isEmpty()) {
        text += tr("PDF引擎: %1\n").arg(m_pdfEngineCombo->currentText());
    }
    if (m_tocCheckBox->isChecked()) {
        text += tr("生成目录 (深度: %1)\n").arg(m_tocDepthSpinBox->value());
    }
    if (m_numberSectionsCheckBox->isChecked()) {
        text += tr("章节编号: 是\n");
    }
    return text;
}
