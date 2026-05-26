#include "conversion_params_dialog.h"
#include "image_params_widget.h"
#include "document_params_widget.h"
#include "audio_params_widget.h"
#include "video_params_widget.h"
#include "config_manager.h"
#include "format_registry.h"
#include "logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFrame>
#include <QMessageBox>
#include <QScrollArea>
#include <QFileInfo>
#include <QDir>

ConversionParamsDialog::ConversionParamsDialog(QWidget* parent)
    : QDialog(parent)
    , m_activeCategory(FormatRegistry::Category::Image)
    , m_categoryCombo(nullptr)
    , m_stackedWidget(nullptr)
    , m_imageParams(nullptr)
    , m_docParams(nullptr)
    , m_audioParams(nullptr)
    , m_videoParams(nullptr)
    , m_okBtn(nullptr)
    , m_cancelBtn(nullptr)
    , m_resetBtn(nullptr)
    , m_statusLabel(nullptr)
    , m_darkMode(false)
{
    setWindowTitle(tr("转换参数设置"));
    setMinimumSize(520, 480);
    setModal(true);

    setupUI();
    setupConnections();
    applyDialogStyleSheet();
    loadPreferences();
}

void ConversionParamsDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // ── Category selector ─────────────────────────────────────────
    QHBoxLayout* selectorRow = new QHBoxLayout();
    QLabel* switchLabel = new QLabel(tr("文件类型:"));
    switchLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #333;");
    selectorRow->addWidget(switchLabel);

    m_categoryCombo = new QComboBox();
    m_categoryCombo->addItem(QString::fromUtf8("\xF0\x9F\x96\xBC") + tr(" 图片"), static_cast<int>(FormatRegistry::Category::Image));
    m_categoryCombo->addItem(QString::fromUtf8("\xF0\x9F\x93\x84") + tr(" 文档"), static_cast<int>(FormatRegistry::Category::Document));
    m_categoryCombo->addItem(QString::fromUtf8("\xF0\x9F\x8E\xB5") + tr(" 音频"), static_cast<int>(FormatRegistry::Category::Audio));
    m_categoryCombo->addItem(QString::fromUtf8("\xF0\x9F\x8E\xAC") + tr(" 视频"), static_cast<int>(FormatRegistry::Category::Video));
    m_categoryCombo->setMinimumHeight(34);
    m_categoryCombo->setMinimumWidth(200);
    m_categoryCombo->setStyleSheet(
        "QComboBox { padding: 6px 12px; border: 2px solid #2196F3; border-radius: 8px; "
        "font-size: 14px; background-color: #fff; }"
        "QComboBox::drop-down { border: none; width: 28px; }"
        "QComboBox QAbstractItemView { border: 1px solid #ccc; border-radius: 4px; "
        "selection-background-color: #e3f2fd; font-size: 13px; }"
    );
    selectorRow->addWidget(m_categoryCombo);
    selectorRow->addStretch();
    mainLayout->addLayout(selectorRow);

    // ── Separator ─────────────────────────────────────────────────
    QFrame* sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("QFrame { color: #e0e0e0; max-height: 1px; }");
    mainLayout->addWidget(sep);

    // ── Scrollable parameter area ─────────────────────────────────
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(
        "QScrollArea { border: none; background: transparent; }"
        "QScrollBar:vertical { background: #f0f0f0; width: 10px; }"
        "QScrollBar::handle:vertical { background: #ccc; border-radius: 5px; min-height: 30px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
    );

    m_stackedWidget = new QStackedWidget();

    m_imageParams = new ImageParamsWidget();
    m_docParams = new DocumentParamsWidget();
    m_audioParams = new AudioParamsWidget();
    m_videoParams = new VideoParamsWidget();

    m_stackedWidget->addWidget(m_imageParams);
    m_stackedWidget->addWidget(m_docParams);
    m_stackedWidget->addWidget(m_audioParams);
    m_stackedWidget->addWidget(m_videoParams);

    scrollArea->setWidget(m_stackedWidget);
    mainLayout->addWidget(scrollArea, 1);

    // ── Status bar ────────────────────────────────────────────────
    m_statusLabel = new QLabel(tr("就绪"));
    m_statusLabel->setStyleSheet("color: #666; font-size: 12px; padding: 2px 0;");
    mainLayout->addWidget(m_statusLabel);

    // ── Button row ────────────────────────────────────────────────
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);

    m_resetBtn = new QPushButton(tr("恢复默认"));
    m_resetBtn->setMinimumHeight(36);
    m_resetBtn->setCursor(Qt::PointingHandCursor);
    m_resetBtn->setStyleSheet(
        "QPushButton { padding: 8px 20px; border: 1px solid #bbb; border-radius: 5px; "
        "background-color: #f5f5f5; color: #555; font-size: 12px; font-weight: bold; }"
        "QPushButton:hover { background-color: #e0e0e0; }"
        "QPushButton:pressed { background-color: #d0d0d0; }"
    );
    btnLayout->addWidget(m_resetBtn);

    btnLayout->addStretch();

    m_okBtn = new QPushButton(tr("确定"));
    m_okBtn->setMinimumHeight(36);
    m_okBtn->setMinimumWidth(100);
    m_okBtn->setCursor(Qt::PointingHandCursor);
    m_okBtn->setStyleSheet(
        "QPushButton { padding: 8px 24px; border: none; border-radius: 5px; "
        "background-color: #1976D2; color: white; font-size: 13px; font-weight: bold; }"
        "QPushButton:hover { background-color: #1565C0; }"
        "QPushButton:pressed { background-color: #0D47A1; }"
    );
    btnLayout->addWidget(m_okBtn);

    m_cancelBtn = new QPushButton(tr("取消"));
    m_cancelBtn->setMinimumHeight(36);
    m_cancelBtn->setMinimumWidth(100);
    m_cancelBtn->setCursor(Qt::PointingHandCursor);
    m_cancelBtn->setStyleSheet(
        "QPushButton { padding: 8px 24px; border: 1px solid #64B5F6; border-radius: 5px; "
        "background-color: #E3F2FD; color: #1565C0; font-size: 13px; font-weight: bold; }"
        "QPushButton:hover { background-color: #BBDEFB; }"
        "QPushButton:pressed { background-color: #90CAF9; }"
    );
    btnLayout->addWidget(m_cancelBtn);

    mainLayout->addLayout(btnLayout);
}

void ConversionParamsDialog::setupConnections() {
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConversionParamsDialog::onCategoryChanged);
    connect(m_okBtn, &QPushButton::clicked, this, &ConversionParamsDialog::onValidateAndAccept);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_resetBtn, &QPushButton::clicked, this, &ConversionParamsDialog::onResetToDefaults);

    // Forward all param change signals
    connect(m_imageParams, &ImageParamsWidget::paramsChanged, this, &ConversionParamsDialog::onAnyParamsChanged);
    connect(m_docParams, &DocumentParamsWidget::paramsChanged, this, &ConversionParamsDialog::onAnyParamsChanged);
    connect(m_audioParams, &AudioParamsWidget::paramsChanged, this, &ConversionParamsDialog::onAnyParamsChanged);
    connect(m_videoParams, &VideoParamsWidget::paramsChanged, this, &ConversionParamsDialog::onAnyParamsChanged);
}

void ConversionParamsDialog::setDarkMode(bool enabled) {
    m_darkMode = enabled;
    applyDialogStyleSheet();
}

void ConversionParamsDialog::applyDialogStyleSheet() {
    if (m_darkMode) {
        setStyleSheet(
            "QDialog { background-color: #1e1e1e; }"
            "QGroupBox { font-weight: bold; border: 1px solid #3c3c3c; border-radius: 8px; "
            "margin-top: 12px; padding: 16px 12px 12px 12px; background-color: #252526; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; "
            "color: #64B5F6; font-size: 13px; }"
            "QGroupBox:focus { border-color: #3c3c3c; }"
            "QLabel { color: #e0e0e0; font-size: 13px; }"
            "QComboBox { padding: 4px 8px; border: 1px solid #555; border-radius: 6px; "
            "background: #333; color: #e0e0e0; font-size: 13px; min-height: 28px; }"
            "QComboBox:focus { border-color: #64B5F6; }"
            "QComboBox::drop-down { border: none; width: 28px; }"
            "QComboBox QAbstractItemView { border: 1px solid #555; border-radius: 4px; "
            "outline: none; background: #333; color: #e0e0e0; selection-background-color: #094771; }"
            "QSpinBox, QDoubleSpinBox { padding: 4px; border: 1px solid #555; border-radius: 6px; "
            "background: #333; color: #e0e0e0; font-size: 13px; min-height: 28px; }"
            "QLineEdit { padding: 4px 8px; border: 1px solid #555; border-radius: 6px; "
            "background: #333; color: #e0e0e0; font-size: 13px; min-height: 28px; }"
            "QCheckBox { spacing: 6px; font-size: 13px; color: #e0e0e0; }"
            "QSlider::groove:horizontal { border: none; background: #444; "
            "height: 6px; border-radius: 3px; }"
            "QSlider::handle:horizontal { background: #64B5F6; border: none; width: 16px; "
            "height: 16px; margin: -6px 0; border-radius: 8px; }"
            "QSlider::sub-page:horizontal { background: #1565C0; border-radius: 3px; }"
            "QScrollArea { border: none; }"
            "QScrollBar:vertical { background: #2d2d2d; width: 10px; }"
            "QScrollBar::handle:vertical { background: #555; border-radius: 5px; min-height: 30px; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        );
        return;
    }
    setStyleSheet(
        "QDialog { background-color: #fafafa; }"
        "QGroupBox { font-weight: bold; border: 1px solid #ddd; border-radius: 8px; "
        "margin-top: 12px; padding: 16px 12px 12px 12px; background-color: #ffffff; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; "
        "color: #1565C0; font-size: 13px; }"
        "QGroupBox:focus { border-color: #ddd; }"
        "QLabel { color: #333; font-size: 13px; }"
        "QComboBox { padding: 4px 8px; border: 1px solid #ccc; border-radius: 6px; "
        "background: #fff; font-size: 13px; min-height: 28px; }"
        "QComboBox:focus { border-color: #1976D2; }"
        "QComboBox::drop-down { border: none; width: 28px; }"
        "QComboBox QAbstractItemView { border: 1px solid #ccc; border-radius: 4px; "
        "outline: none; selection-background-color: #e3f2fd; }"
        "QSpinBox, QDoubleSpinBox { padding: 4px; border: 1px solid #ccc; border-radius: 6px; "
        "background: #fff; font-size: 13px; min-height: 28px; }"
        "QSpinBox:focus, QDoubleSpinBox:focus { border-color: #1976D2; }"
        "QLineEdit { padding: 4px 8px; border: 1px solid #ccc; border-radius: 6px; "
        "background: #fff; font-size: 13px; min-height: 28px; }"
        "QLineEdit:focus { border-color: #1976D2; }"
        "QCheckBox { spacing: 6px; font-size: 13px; }"
        "QCheckBox:focus { outline: none; }"
        "QSlider::groove:horizontal { border: none; background: #e0e0e0; "
        "height: 6px; border-radius: 3px; }"
        "QSlider::handle:horizontal { background: #1976D2; border: none; width: 16px; "
        "height: 16px; margin: -6px 0; border-radius: 8px; }"
        "QSlider::sub-page:horizontal { background: #90CAF9; border-radius: 3px; }"
        "QScrollArea { border: none; }"
    );
}

void ConversionParamsDialog::setActiveCategory(FormatRegistry::Category category) {
    m_activeCategory = category;
    int idx = 0;
    switch (category) {
        case FormatRegistry::Category::Image:    idx = 0; break;
        case FormatRegistry::Category::Document:  idx = 1; break;
        case FormatRegistry::Category::Audio:     idx = 2; break;
        case FormatRegistry::Category::Video:     idx = 3; break;
        default: idx = 0; break;
    }
    m_categoryCombo->setCurrentIndex(idx);
    m_stackedWidget->setCurrentIndex(idx);
}

void ConversionParamsDialog::onCategoryChanged(int index) {
    switch (index) {
        case 0: m_activeCategory = FormatRegistry::Category::Image; break;
        case 1: m_activeCategory = FormatRegistry::Category::Document; break;
        case 2: m_activeCategory = FormatRegistry::Category::Audio; break;
        case 3: m_activeCategory = FormatRegistry::Category::Video; break;
        default: break;
    }
    m_stackedWidget->setCurrentIndex(index);
    m_statusLabel->setText(tr("已切换到 %1 参数").arg(m_categoryCombo->currentText().trimmed()));
}

void ConversionParamsDialog::onAnyParamsChanged() {
    m_statusLabel->setText(tr("参数已更新"));
}

void ConversionParamsDialog::onValidateAndAccept() {
    // Validate all parameter widgets
    QStringList allErrors;
    allErrors << m_imageParams->validate();
    allErrors << m_docParams->validate();
    allErrors << m_audioParams->validate();
    allErrors << m_videoParams->validate();

    if (!allErrors.isEmpty()) {
        QString errorMsg = tr("参数验证失败:\n");
        for (const QString& err : allErrors) {
            errorMsg += "• " + err + "\n";
        }
        m_statusLabel->setStyleSheet("color: #d32f2f; font-size: 12px; padding: 2px 0;");
        m_statusLabel->setText(tr("验证失败: %1 个错误").arg(allErrors.size()));
        QMessageBox::warning(this, tr("参数验证错误"), errorMsg);
        return;
    }

    // Reset status label to default style on success
    m_statusLabel->setStyleSheet("color: #666; font-size: 12px; padding: 2px 0;");

    // Save params for all categories
    m_savedParams[FormatRegistry::Category::Image] = m_imageParams->getParams();
    m_savedParams[FormatRegistry::Category::Document] = m_docParams->getParams();
    m_savedParams[FormatRegistry::Category::Audio] = m_audioParams->getParams();
    m_savedParams[FormatRegistry::Category::Video] = m_videoParams->getParams();

    // Save preferences
    savePreferences();

    LOG_INFO("ConversionParamsDialog", "转换参数设置已保存");

    emit paramsAccepted(m_activeCategory, getParams());
    accept();
}

void ConversionParamsDialog::onResetToDefaults() {
    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("确认"),
        tr("确定要恢复所有参数到默认值吗？"),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::No) return;

    // Reset each widget by passing empty params
    m_imageParams->setParams(QVariantMap());
    m_docParams->setParams(QVariantMap());
    m_audioParams->setParams(QVariantMap());
    m_videoParams->setParams(QVariantMap());

    m_statusLabel->setStyleSheet("color: #2e7d32; font-size: 12px; padding: 2px 0;");
    m_statusLabel->setText(tr("已恢复默认设置"));

    LOG_INFO("ConversionParamsDialog", "参数已恢复默认");
}

QVariantMap ConversionParamsDialog::getParams() const {
    return getParamsForCategory(m_activeCategory);
}

QVariantMap ConversionParamsDialog::getParamsForCategory(FormatRegistry::Category category) const {
    if (m_savedParams.contains(category)) {
        return m_savedParams[category];
    }
    return QVariantMap();
}

void ConversionParamsDialog::setParamsForCategory(FormatRegistry::Category category, const QVariantMap& params) {
    m_savedParams[category] = params;
    switch (category) {
        case FormatRegistry::Category::Image:
            m_imageParams->setParams(params);
            break;
        case FormatRegistry::Category::Document:
            m_docParams->setParams(params);
            break;
        case FormatRegistry::Category::Audio:
            m_audioParams->setParams(params);
            break;
        case FormatRegistry::Category::Video:
            m_videoParams->setParams(params);
            break;
    }
}

void ConversionParamsDialog::loadPreferences() {
    ConfigManager& cm = ConfigManager::instance();

    // Load per-category params from config
    auto loadCat = [&](FormatRegistry::Category cat, const QString& key) {
        QVariantMap params = cm.value(key).toMap();
        if (!params.isEmpty()) {
            setParamsForCategory(cat, params);
        }
    };

    loadCat(FormatRegistry::Category::Image, "params_image");
    loadCat(FormatRegistry::Category::Document, "params_document");
    loadCat(FormatRegistry::Category::Audio, "params_audio");
    loadCat(FormatRegistry::Category::Video, "params_video");

    // Also restore saved maps
    m_savedParams[FormatRegistry::Category::Image] = m_imageParams->getParams();
    m_savedParams[FormatRegistry::Category::Document] = m_docParams->getParams();
    m_savedParams[FormatRegistry::Category::Audio] = m_audioParams->getParams();
    m_savedParams[FormatRegistry::Category::Video] = m_videoParams->getParams();

    m_statusLabel->setText(tr("已加载保存的参数设置"));
}

void ConversionParamsDialog::savePreferences() {
    ConfigManager& cm = ConfigManager::instance();

    cm.setValue("params_image", QVariant(m_imageParams->getParams()));
    cm.setValue("params_document", QVariant(m_docParams->getParams()));
    cm.setValue("params_audio", QVariant(m_audioParams->getParams()));
    cm.setValue("params_video", QVariant(m_videoParams->getParams()));

    // Persist to disk (use saved config path or fallback to default)
    QString configPath = cm.value("configFilePath").toString();
    if (configPath.isEmpty()) {
        configPath = QDir::homePath() + "/.integrated_converter/config.json";
    }
    cm.saveConfig(configPath);

    LOG_INFO("ConversionParamsDialog", "参数偏好已保存到配置文件");
}


