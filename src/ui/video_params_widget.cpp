#include "video_params_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFrame>
#include <QRegularExpressionValidator>

VideoParamsWidget::VideoParamsWidget(QWidget* parent)
    : QWidget(parent)
    , m_videoCodecCombo(nullptr)
    , m_audioCodecCombo(nullptr)
    , m_resolutionInput(nullptr)
    , m_videoBitrateCombo(nullptr)
    , m_audioBitrateCombo(nullptr)
    , m_framerateCombo(nullptr)
    , m_presetCombo(nullptr)
    , m_crfSlider(nullptr)
    , m_crfSpinBox(nullptr)
    , m_twoPassCheckBox(nullptr)
    , m_previewLabel(nullptr)
{
    setupUI();
    setupConnections();
}

void VideoParamsWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // ── Codecs ────────────────────────────────────────────────────
    QGroupBox* codecGroup = new QGroupBox(tr("编码器选择"));
    QGridLayout* codecGrid = new QGridLayout(codecGroup);
    codecGrid->setSpacing(8);

    codecGrid->addWidget(new QLabel(tr("视频编码器:")), 0, 0);
    m_videoCodecCombo = new QComboBox();
    m_videoCodecCombo->addItem("H.264 (AVC)", "h264");
    m_videoCodecCombo->addItem("H.265 (HEVC)", "hevc");
    m_videoCodecCombo->addItem("VP9", "vp9");
    m_videoCodecCombo->addItem("AV1", "av1");
    m_videoCodecCombo->addItem("MPEG-4", "mpeg4");
    m_videoCodecCombo->addItem(tr("自动"), "auto");
    m_videoCodecCombo->setMinimumHeight(32);
    m_videoCodecCombo->setStyleSheet(
        "QComboBox { padding: 3px 8px; border: 2px solid #1976D2; border-radius: 5px; "
        "background-color: #ffffff; color: #333; font-size: 12px; min-width: 100px; }"
        "QComboBox::drop-down { border: none; width: 22px; "
        "background-color: #ffffff; }"
        "QComboBox::down-arrow { width: 10px; height: 10px; }"
        "QComboBox QAbstractItemView { "
        "border: 1px solid #ccc; border-radius: 4px; background-color: #ffffff; "
        "color: #333; selection-background-color: #e3f2fd; selection-color: #000; "
        "font-size: 12px; }"
    );
    codecGrid->addWidget(m_videoCodecCombo, 0, 1);

    codecGrid->addWidget(new QLabel(tr("音频编码器:")), 1, 0);
    m_audioCodecCombo = new QComboBox();
    m_audioCodecCombo->addItem("AAC", "aac");
    m_audioCodecCombo->addItem("MP3", "mp3");
    m_audioCodecCombo->addItem(tr("复制源音频"), "copy");
    m_audioCodecCombo->addItem(tr("无音频"), "none");
    m_audioCodecCombo->setMinimumHeight(30);
    codecGrid->addWidget(m_audioCodecCombo, 1, 1);

    mainLayout->addWidget(codecGroup);

    // ── Quality ───────────────────────────────────────────────────
    QGroupBox* qualityGroup = new QGroupBox(tr("质量 / CRF设置"));
    QVBoxLayout* qualityLayout = new QVBoxLayout(qualityGroup);

    QHBoxLayout* crfRow = new QHBoxLayout();
    crfRow->addWidget(new QLabel(tr("CRF值 (0-51):")));

    m_crfSlider = new QSlider(Qt::Horizontal);
    m_crfSlider->setRange(0, 51);
    m_crfSlider->setValue(23);
    m_crfSlider->setTickPosition(QSlider::TicksBelow);
    m_crfSlider->setTickInterval(5);
    crfRow->addWidget(m_crfSlider, 1);

    m_crfSpinBox = new QSpinBox();
    m_crfSpinBox->setRange(0, 51);
    m_crfSpinBox->setValue(23);
    m_crfSpinBox->setFixedWidth(70);
    crfRow->addWidget(m_crfSpinBox);

    qualityLayout->addLayout(crfRow);

    QLabel* crfHint = new QLabel(tr("提示: 0=无损, 23=默认, 51=最差质量/最小文件"));
    crfHint->setStyleSheet("color: #888; font-size: 11px;");
    qualityLayout->addWidget(crfHint);

    mainLayout->addWidget(qualityGroup);

    // ── Resolution & Bitrate ──────────────────────────────────────
    QGroupBox* resGroup = new QGroupBox(tr("分辨率与码率"));
    QGridLayout* resGrid = new QGridLayout(resGroup);
    resGrid->setSpacing(8);

    resGrid->addWidget(new QLabel(tr("分辨率:")), 0, 0);
    m_resolutionInput = new QLineEdit();
    m_resolutionInput->setPlaceholderText(tr("例如: 1920x1080, 1280x720, 留空=原始"));
    m_resolutionInput->setMaxLength(21);
    m_resolutionInput->setMinimumHeight(30);
    m_resolutionInput->setStyleSheet(
        "QLineEdit { padding: 4px 8px; border: 1px solid #ccc; border-radius: 6px; font-size: 13px; }"
    );
    QRegularExpression resRx(R"(^\d+[xX]\d+$|^$)");
    m_resolutionInput->setValidator(new QRegularExpressionValidator(resRx, this));
    resGrid->addWidget(m_resolutionInput, 0, 1);

    resGrid->addWidget(new QLabel(tr("视频码率:")), 1, 0);
    m_videoBitrateCombo = new QComboBox();
    m_videoBitrateCombo->addItem(tr("自动"), "");
    m_videoBitrateCombo->addItem("500 kbps", "500k");
    m_videoBitrateCombo->addItem("1 Mbps", "1M");
    m_videoBitrateCombo->addItem("2 Mbps", "2M");
    m_videoBitrateCombo->addItem("5 Mbps", "5M");
    m_videoBitrateCombo->addItem("10 Mbps", "10M");
    m_videoBitrateCombo->addItem("20 Mbps", "20M");
    m_videoBitrateCombo->addItem("50 Mbps", "50M");
    m_videoBitrateCombo->setMinimumHeight(30);
    resGrid->addWidget(m_videoBitrateCombo, 1, 1);

    resGrid->addWidget(new QLabel(tr("音频码率:")), 2, 0);
    m_audioBitrateCombo = new QComboBox();
    m_audioBitrateCombo->addItem(tr("自动"), "");
    m_audioBitrateCombo->addItem("64 kbps", "64k");
    m_audioBitrateCombo->addItem("128 kbps", "128k");
    m_audioBitrateCombo->addItem("192 kbps", "192k");
    m_audioBitrateCombo->addItem("256 kbps", "256k");
    m_audioBitrateCombo->addItem("320 kbps", "320k");
    m_audioBitrateCombo->setMinimumHeight(30);
    resGrid->addWidget(m_audioBitrateCombo, 2, 1);

    resGrid->addWidget(new QLabel(tr("帧率:")), 3, 0);
    m_framerateCombo = new QComboBox();
    m_framerateCombo->addItem(tr("原始"), "");
    m_framerateCombo->addItem("24 fps", "24");
    m_framerateCombo->addItem("25 fps", "25");
    m_framerateCombo->addItem("30 fps", "30");
    m_framerateCombo->addItem("48 fps", "48");
    m_framerateCombo->addItem("60 fps", "60");
    m_framerateCombo->setMinimumHeight(30);
    resGrid->addWidget(m_framerateCombo, 3, 1);

    mainLayout->addWidget(resGroup);

    // ── Preset & Advanced ─────────────────────────────────────────
    QGroupBox* advGroup = new QGroupBox(tr("高级选项"));
    QGridLayout* advGrid = new QGridLayout(advGroup);
    advGrid->setSpacing(8);

    advGrid->addWidget(new QLabel(tr("编码预设:")), 0, 0);
    m_presetCombo = new QComboBox();
    m_presetCombo->addItem("ultrafast", "ultrafast");
    m_presetCombo->addItem("superfast", "superfast");
    m_presetCombo->addItem("veryfast", "veryfast");
    m_presetCombo->addItem("faster", "faster");
    m_presetCombo->addItem("fast", "fast");
    m_presetCombo->addItem("medium", "medium");
    m_presetCombo->addItem("slow", "slow");
    m_presetCombo->addItem("slower", "slower");
    m_presetCombo->addItem("veryslow", "veryslow");
    m_presetCombo->setCurrentText("medium");
    m_presetCombo->setMinimumHeight(30);
    advGrid->addWidget(m_presetCombo, 0, 1);

    m_twoPassCheckBox = new QCheckBox(tr("二遍编码 (2-Pass) — 提高码率分配精度"));
    m_twoPassCheckBox->setStyleSheet("font-size: 13px;");
    advGrid->addWidget(m_twoPassCheckBox, 1, 0, 1, 2);

    QLabel* presetHint = new QLabel(tr("提示: ultrafast=编码快但文件大, veryslow=编码慢但文件小"));
    presetHint->setStyleSheet("color: #888; font-size: 11px;");
    advGrid->addWidget(presetHint, 2, 0, 1, 2);

    mainLayout->addWidget(advGroup);

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

void VideoParamsWidget::setupConnections() {
    auto updatePreview = [this]() {
        m_previewLabel->setText(buildPreviewText());
        emit paramsChanged();
    };

    connect(m_videoCodecCombo, &QComboBox::currentTextChanged, this, updatePreview);
    connect(m_audioCodecCombo, &QComboBox::currentTextChanged, this, updatePreview);
    connect(m_resolutionInput, &QLineEdit::textChanged, this, updatePreview);
    connect(m_videoBitrateCombo, &QComboBox::currentTextChanged, this, updatePreview);
    connect(m_audioBitrateCombo, &QComboBox::currentTextChanged, this, updatePreview);
    connect(m_framerateCombo, &QComboBox::currentTextChanged, this, updatePreview);
    connect(m_presetCombo, &QComboBox::currentTextChanged, this, updatePreview);
    connect(m_crfSlider, &QSlider::valueChanged, m_crfSpinBox, &QSpinBox::setValue);
    connect(m_crfSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), m_crfSlider, &QSlider::setValue);
    connect(m_crfSlider, &QSlider::valueChanged, this, updatePreview);
    connect(m_twoPassCheckBox, &QCheckBox::toggled, this, updatePreview);
}

QVariantMap VideoParamsWidget::getParams() const {
    QVariantMap params;
    params["videoCodec"] = m_videoCodecCombo->currentData().toString();
    params["audioCodec"] = m_audioCodecCombo->currentData().toString();
    QString res = m_resolutionInput->text().trimmed();
    if (!res.isEmpty()) {
        params["resolution"] = res;
    }
    QString vbitrate = m_videoBitrateCombo->currentData().toString();
    if (!vbitrate.isEmpty()) {
        params["videoBitrate"] = vbitrate;
    }
    QString abitrate = m_audioBitrateCombo->currentData().toString();
    if (!abitrate.isEmpty()) {
        params["audioBitrate"] = abitrate;
    }
    QString fps = m_framerateCombo->currentData().toString();
    if (!fps.isEmpty()) {
        params["framerate"] = fps;
    }
    params["preset"] = m_presetCombo->currentData().toString();
    params["crf"] = m_crfSpinBox->value();
    params["twoPass"] = m_twoPassCheckBox->isChecked();
    return params;
}

void VideoParamsWidget::setParams(const QVariantMap& params) {
    m_videoCodecCombo->blockSignals(true);
    m_audioCodecCombo->blockSignals(true);
    m_resolutionInput->blockSignals(true);
    m_videoBitrateCombo->blockSignals(true);
    m_audioBitrateCombo->blockSignals(true);
    m_framerateCombo->blockSignals(true);
    m_presetCombo->blockSignals(true);
    m_crfSlider->blockSignals(true);
    m_crfSpinBox->blockSignals(true);
    m_twoPassCheckBox->blockSignals(true);

    auto setComboData = [](QComboBox* combo, const QVariant& data) {
        int idx = combo->findData(data);
        if (idx >= 0) combo->setCurrentIndex(idx);
    };

    setComboData(m_videoCodecCombo, params.value("videoCodec", "h264"));
    setComboData(m_audioCodecCombo, params.value("audioCodec", "aac"));
    m_resolutionInput->setText(params.value("resolution").toString());
    setComboData(m_videoBitrateCombo, params.value("videoBitrate", ""));
    setComboData(m_audioBitrateCombo, params.value("audioBitrate", ""));
    setComboData(m_framerateCombo, params.value("framerate", ""));
    setComboData(m_presetCombo, params.value("preset", "medium"));
    m_crfSpinBox->setValue(params.value("crf", 23).toInt());
    m_twoPassCheckBox->setChecked(params.value("twoPass", false).toBool());

    m_videoCodecCombo->blockSignals(false);
    m_audioCodecCombo->blockSignals(false);
    m_resolutionInput->blockSignals(false);
    m_videoBitrateCombo->blockSignals(false);
    m_audioBitrateCombo->blockSignals(false);
    m_framerateCombo->blockSignals(false);
    m_presetCombo->blockSignals(false);
    m_crfSlider->blockSignals(false);
    m_crfSpinBox->blockSignals(false);
    m_twoPassCheckBox->blockSignals(false);

    m_previewLabel->setText(buildPreviewText());
}

QStringList VideoParamsWidget::validate() const {
    QStringList errors;
    int crf = m_crfSpinBox->value();
    if (crf < 0 || crf > 51) {
        errors << tr("CRF值必须在 0-51 之间 (当前: %1)").arg(crf);
    }
    QString res = m_resolutionInput->text().trimmed();
    if (!res.isEmpty()) {
        QStringList parts = res.split(QRegularExpression(R"([xX])"));
        if (parts.size() == 2) {
            bool ok1, ok2;
            int w = parts[0].toInt(&ok1);
            int h = parts[1].toInt(&ok2);
            if (!ok1 || !ok2 || w <= 0 || h <= 0) {
                errors << tr("分辨率格式无效: \"%1\" (应为 WIDTHxHEIGHT, 如 1920x1080)").arg(res);
            }
        } else {
            errors << tr("分辨率格式无效: \"%1\"").arg(res);
        }
    }
    return errors;
}

QString VideoParamsWidget::buildPreviewText() const {
    QString text = tr("📋 视频参数预览\n");
    text += tr("━━━━━━━━━━━━━━━━━━\n");
    text += tr("视频编码器: %1\n").arg(m_videoCodecCombo->currentText());
    text += tr("音频编码器: %1\n").arg(m_audioCodecCombo->currentText());
    text += tr("CRF: %1 (0=无损, 51=最差)\n").arg(m_crfSpinBox->value());
    QString res = m_resolutionInput->text().trimmed();
    if (!res.isEmpty()) {
        text += tr("分辨率: %1\n").arg(res);
    }
    if (!m_videoBitrateCombo->currentData().toString().isEmpty()) {
        text += tr("视频码率: %1\n").arg(m_videoBitrateCombo->currentText());
    }
    if (!m_audioBitrateCombo->currentData().toString().isEmpty()) {
        text += tr("音频码率: %1\n").arg(m_audioBitrateCombo->currentText());
    }
    if (!m_framerateCombo->currentData().toString().isEmpty()) {
        text += tr("帧率: %1\n").arg(m_framerateCombo->currentText());
    }
    text += tr("预设: %1\n").arg(m_presetCombo->currentText());
    if (m_twoPassCheckBox->isChecked()) {
        text += tr("二遍编码: 是\n");
    }
    return text;
}
