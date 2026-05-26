#include "image_params_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFrame>
#include <QRegularExpression>
#include <QFont>

ImageParamsWidget::ImageParamsWidget(QWidget* parent)
    : QWidget(parent)
    , m_qualitySlider(nullptr)
    , m_qualitySpinBox(nullptr)
    , m_resizeInput(nullptr)
    , m_compressionCombo(nullptr)
    , m_densitySpinBox(nullptr)
    , m_stripCheckBox(nullptr)
    , m_depthCombo(nullptr)
    , m_previewLabel(nullptr)
{
    setupUI();
    setupConnections();
}

void ImageParamsWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // ── Quality ───────────────────────────────────────────────────
    QGroupBox* qualityGroup = new QGroupBox(tr("质量设置"));
    QVBoxLayout* qualityLayout = new QVBoxLayout(qualityGroup);

    QHBoxLayout* qualityRow = new QHBoxLayout();
    QLabel* qualityLabel = new QLabel(tr("质量:"));
    qualityLabel->setMinimumWidth(50);
    qualityRow->addWidget(qualityLabel);

    m_qualitySlider = new QSlider(Qt::Horizontal);
    m_qualitySlider->setRange(1, 100);
    m_qualitySlider->setValue(85);
    m_qualitySlider->setTickPosition(QSlider::TicksBelow);
    m_qualitySlider->setTickInterval(10);
    qualityRow->addWidget(m_qualitySlider, 1);

    m_qualitySpinBox = new QSpinBox();
    m_qualitySpinBox->setRange(1, 100);
    m_qualitySpinBox->setValue(85);
    m_qualitySpinBox->setFixedWidth(60);
    m_qualitySpinBox->setSuffix(tr("%"));
    qualityRow->addWidget(m_qualitySpinBox);

    qualityLayout->addLayout(qualityRow);
    mainLayout->addWidget(qualityGroup);

    // ── Resolution ────────────────────────────────────────────────
    QGroupBox* resizeGroup = new QGroupBox(tr("分辨率 / 缩放"));
    QHBoxLayout* resizeLayout = new QHBoxLayout(resizeGroup);

    QLabel* resizeLabel = new QLabel(tr("缩放:"));
    resizeLabel->setMinimumWidth(50);
    resizeLayout->addWidget(resizeLabel);

    m_resizeInput = new QLineEdit();
    m_resizeInput->setPlaceholderText(tr("例如: 800x600, 50%, x1080"));
    m_resizeInput->setMaxLength(21);
    m_resizeInput->setMinimumHeight(32);
    m_resizeInput->setStyleSheet(
        "QLineEdit { padding: 4px 8px; border: 1px solid #ccc; border-radius: 6px; font-size: 13px; }"
    );
    // Validation
    QRegularExpression resizeRx(R"(^\d+[xX]\d+!?$|^\d+%$|^x\d+$|^\d+$|^$)");
    m_resizeInput->setValidator(new QRegularExpressionValidator(resizeRx, this));
    resizeLayout->addWidget(m_resizeInput, 1);

    QLabel* hintLabel = new QLabel(tr("留空则不缩放"));
    hintLabel->setStyleSheet("color: #999; font-size: 11px;");
    resizeLayout->addWidget(hintLabel);

    mainLayout->addWidget(resizeGroup);

    // ── Compression ───────────────────────────────────────────────
    QGroupBox* compGroup = new QGroupBox(tr("压缩选项"));
    QGridLayout* compGrid = new QGridLayout(compGroup);
    compGrid->setSpacing(8);

    compGrid->addWidget(new QLabel(tr("压缩方式:")), 0, 0);
    m_compressionCombo = new QComboBox();
    m_compressionCombo->addItem(tr("无"), "");
    m_compressionCombo->addItem("JPEG", "JPEG");
    m_compressionCombo->addItem("LZW", "LZW");
    m_compressionCombo->addItem("RLE", "RLE");
    m_compressionCombo->addItem("Zip", "Zip");
    m_compressionCombo->setMinimumHeight(30);
    compGrid->addWidget(m_compressionCombo, 0, 1);

    compGrid->addWidget(new QLabel(tr("DPI:")), 1, 0);
    m_densitySpinBox = new QSpinBox();
    m_densitySpinBox->setRange(0, 12000);
    m_densitySpinBox->setValue(0);
    m_densitySpinBox->setSuffix(tr(" dpi"));
    m_densitySpinBox->setSpecialValueText(tr("默认"));
    m_densitySpinBox->setMinimumHeight(30);
    compGrid->addWidget(m_densitySpinBox, 1, 1);

    compGrid->addWidget(new QLabel(tr("位深度:")), 2, 0);
    m_depthCombo = new QComboBox();
    m_depthCombo->addItem(tr("默认"), "");
    m_depthCombo->addItem("8", "8");
    m_depthCombo->addItem("16", "16");
    m_depthCombo->addItem("32", "32");
    m_depthCombo->setMinimumHeight(30);
    compGrid->addWidget(m_depthCombo, 2, 1);

    mainLayout->addWidget(compGroup);

    // ── Advanced ──────────────────────────────────────────────────
    m_stripCheckBox = new QCheckBox(tr("清理元数据 (Strip) — 移除EXIF等信息以减小文件体积"));
    m_stripCheckBox->setStyleSheet("font-size: 13px;");
    mainLayout->addWidget(m_stripCheckBox);

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
    m_previewLabel->setMinimumHeight(120);
    previewLayout->addWidget(m_previewLabel);
    mainLayout->addWidget(previewGroup);

    mainLayout->addStretch();
}

void ImageParamsWidget::setupConnections() {
    connect(m_qualitySlider, &QSlider::valueChanged, m_qualitySpinBox, &QSpinBox::setValue);
    connect(m_qualitySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), m_qualitySlider, &QSlider::setValue);

    connect(m_qualitySlider, &QSlider::valueChanged, this, [this]() {
        m_previewLabel->setText(buildPreviewText());
        emit paramsChanged();
    });
    connect(m_resizeInput, &QLineEdit::textChanged, this, [this]() {
        m_previewLabel->setText(buildPreviewText());
        emit paramsChanged();
    });
    connect(m_compressionCombo, &QComboBox::currentTextChanged, this, [this]() {
        m_previewLabel->setText(buildPreviewText());
        emit paramsChanged();
    });
    connect(m_densitySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        m_previewLabel->setText(buildPreviewText());
        emit paramsChanged();
    });
    connect(m_stripCheckBox, &QCheckBox::toggled, this, [this]() {
        m_previewLabel->setText(buildPreviewText());
        emit paramsChanged();
    });
    connect(m_depthCombo, &QComboBox::currentTextChanged, this, [this]() {
        m_previewLabel->setText(buildPreviewText());
        emit paramsChanged();
    });
}

void ImageParamsWidget::setEnabledFormats(const QStringList& formats) {
    Q_UNUSED(formats);
    // Format selection is handled by the main window's config panel.
    // This widget focuses on conversion parameters only.
    m_previewLabel->setText(buildPreviewText());
}

QVariantMap ImageParamsWidget::getParams() const {
    QVariantMap params;
    params["quality"] = m_qualitySpinBox->value();
    QString resizeText = m_resizeInput->text().trimmed();
    if (!resizeText.isEmpty()) {
        params["resize"] = resizeText;
    }
    params["compression"] = m_compressionCombo->currentData().toString();
    int density = m_densitySpinBox->value();
    if (density > 0) {
        params["density"] = density;
    }
    params["strip"] = m_stripCheckBox->isChecked();
    params["depth"] = m_depthCombo->currentData().toString();
    return params;
}

void ImageParamsWidget::setParams(const QVariantMap& params) {
    m_qualitySlider->blockSignals(true);
    m_qualitySpinBox->blockSignals(true);
    m_resizeInput->blockSignals(true);
    m_compressionCombo->blockSignals(true);
    m_densitySpinBox->blockSignals(true);
    m_stripCheckBox->blockSignals(true);
    m_depthCombo->blockSignals(true);

    m_qualitySpinBox->setValue(params.value("quality", 85).toInt());
    m_resizeInput->setText(params.value("resize").toString());

    QString comp = params.value("compression").toString();
    int compIdx = m_compressionCombo->findData(comp);
    if (compIdx >= 0) m_compressionCombo->setCurrentIndex(compIdx);

    m_densitySpinBox->setValue(params.value("density", 0).toInt());
    m_stripCheckBox->setChecked(params.value("strip", false).toBool());

    QString depth = params.value("depth").toString();
    int depthIdx = m_depthCombo->findData(depth);
    if (depthIdx >= 0) m_depthCombo->setCurrentIndex(depthIdx);

    m_qualitySlider->blockSignals(false);
    m_qualitySpinBox->blockSignals(false);
    m_resizeInput->blockSignals(false);
    m_compressionCombo->blockSignals(false);
    m_densitySpinBox->blockSignals(false);
    m_stripCheckBox->blockSignals(false);
    m_depthCombo->blockSignals(false);

    m_previewLabel->setText(buildPreviewText());
}

QStringList ImageParamsWidget::validate() const {
    QStringList errors;
    int quality = m_qualitySpinBox->value();
    if (quality < 1 || quality > 100) {
        errors << tr("质量值必须在 1-100 之间 (当前: %1)").arg(quality);
    }
    QString resize = m_resizeInput->text().trimmed();
    if (!resize.isEmpty()) {
        static const QRegularExpression resizeRe(R"(^\d+[xX]\d+!?$|^\d+%$|^x\d+$|^\d+$)");
        if (!resizeRe.match(resize).hasMatch()) {
            errors << tr("缩放格式无效: \"%1\" (应为 800x600, 50%, x1080 或 800)").arg(resize);
        }
    }
    int density = m_densitySpinBox->value();
    if (density < 0 || density > 12000) {
        errors << tr("DPI 超出范围 (0-12000)");
    }
    return errors;
}

QString ImageParamsWidget::buildPreviewText() const {
    QString text = tr("📋 图片参数预览\n");
    text += tr("━━━━━━━━━━━━━━━━━━\n");
    text += tr("质量: %1%\n").arg(m_qualitySpinBox->value());
    QString resize = m_resizeInput->text().trimmed();
    if (!resize.isEmpty()) {
        text += tr("缩放: %1\n").arg(resize);
    }
    QString comp = m_compressionCombo->currentText();
    if (comp != tr("无")) {
        text += tr("压缩: %1\n").arg(comp);
    }
    if (m_densitySpinBox->value() > 0) {
        text += tr("DPI: %1\n").arg(m_densitySpinBox->value());
    }
    if (m_stripCheckBox->isChecked()) {
        text += tr("清理元数据: 是\n");
    }
    QString depth = m_depthCombo->currentText();
    if (depth != tr("默认")) {
        text += tr("位深度: %1\n").arg(depth);
    }
    return text;
}
