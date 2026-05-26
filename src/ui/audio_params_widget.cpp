#include "audio_params_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFrame>

AudioParamsWidget::AudioParamsWidget(QWidget* parent)
    : QWidget(parent)
    , m_bitrateCombo(nullptr)
    , m_sampleRateCombo(nullptr)
    , m_channelsCombo(nullptr)
    , m_vbrQualitySlider(nullptr)
    , m_vbrQualitySpinBox(nullptr)
    , m_codecCombo(nullptr)
    , m_previewLabel(nullptr)
{
    setupUI();
    setupConnections();
}

void AudioParamsWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // ── Codec ─────────────────────────────────────────────────────
    QGroupBox* codecGroup = new QGroupBox(tr("编码器"));
    QHBoxLayout* codecLayout = new QHBoxLayout(codecGroup);
    m_codecCombo = new QComboBox();
    m_codecCombo->addItem("AAC (Advanced Audio Coding)", "aac");
    m_codecCombo->addItem("MP3 (MPEG Audio Layer 3)", "mp3");
    m_codecCombo->addItem("FLAC (Free Lossless Audio Codec)", "flac");
    m_codecCombo->addItem("Vorbis (OGG)", "vorbis");
    m_codecCombo->addItem("Opus", "opus");
    m_codecCombo->addItem("WAV (PCM)", "pcm_s16le");
    m_codecCombo->setMinimumHeight(32);
    m_codecCombo->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 2px solid #2196F3; border-radius: 6px; font-size: 14px; }"
        "QComboBox::drop-down { border: none; width: 28px; }"
        "QComboBox QAbstractItemView { border: 1px solid #ccc; border-radius: 4px; "
        "selection-background-color: #e3f2fd; font-size: 13px; }"
    );
    codecLayout->addWidget(m_codecCombo, 1);
    mainLayout->addWidget(codecGroup);

    // ── Bitrate ───────────────────────────────────────────────────
    QGroupBox* bitrateGroup = new QGroupBox(tr("比特率设置"));
    QGridLayout* bitrateGrid = new QGridLayout(bitrateGroup);
    bitrateGrid->setSpacing(8);

    bitrateGrid->addWidget(new QLabel(tr("比特率:")), 0, 0);
    m_bitrateCombo = new QComboBox();
    m_bitrateCombo->addItem(tr("自动"), "auto");
    m_bitrateCombo->addItem("64 kbps", "64k");
    m_bitrateCombo->addItem("96 kbps", "96k");
    m_bitrateCombo->addItem("128 kbps", "128k");
    m_bitrateCombo->addItem("192 kbps", "192k");
    m_bitrateCombo->addItem("256 kbps", "256k");
    m_bitrateCombo->addItem("320 kbps", "320k");
    m_bitrateCombo->setMinimumHeight(30);
    bitrateGrid->addWidget(m_bitrateCombo, 0, 1);

    bitrateGrid->addWidget(new QLabel(tr("采样率:")), 1, 0);
    m_sampleRateCombo = new QComboBox();
    m_sampleRateCombo->addItem(tr("自动"), 0);
    m_sampleRateCombo->addItem("22050 Hz", 22050);
    m_sampleRateCombo->addItem("44100 Hz", 44100);
    m_sampleRateCombo->addItem("48000 Hz", 48000);
    m_sampleRateCombo->addItem("96000 Hz", 96000);
    m_sampleRateCombo->addItem("192000 Hz", 192000);
    m_sampleRateCombo->setMinimumHeight(30);
    bitrateGrid->addWidget(m_sampleRateCombo, 1, 1);

    bitrateGrid->addWidget(new QLabel(tr("声道:")), 2, 0);
    m_channelsCombo = new QComboBox();
    m_channelsCombo->addItem(tr("自动"), 0);
    m_channelsCombo->addItem(tr("单声道 (Mono)"), 1);
    m_channelsCombo->addItem(tr("立体声 (Stereo)"), 2);
    m_channelsCombo->addItem(tr("环绕声 5.1"), 6);
    m_channelsCombo->setMinimumHeight(30);
    bitrateGrid->addWidget(m_channelsCombo, 2, 1);

    // VBR quality
    QHBoxLayout* vbrLayout = new QHBoxLayout();
    vbrLayout->addWidget(new QLabel(tr("VBR质量:")));
    m_vbrQualitySlider = new QSlider(Qt::Horizontal);
    m_vbrQualitySlider->setRange(0, 9);
    m_vbrQualitySlider->setValue(5);
    m_vbrQualitySlider->setTickPosition(QSlider::TicksBelow);
    m_vbrQualitySlider->setTickInterval(1);
    vbrLayout->addWidget(m_vbrQualitySlider, 1);

    m_vbrQualitySpinBox = new QSpinBox();
    m_vbrQualitySpinBox->setRange(0, 9);
    m_vbrQualitySpinBox->setValue(5);
    m_vbrQualitySpinBox->setFixedWidth(60);
    m_vbrQualitySpinBox->setToolTip(tr("0=最高质量, 9=最高压缩"));
    vbrLayout->addWidget(m_vbrQualitySpinBox);

    bitrateGrid->addLayout(vbrLayout, 3, 0, 1, 2);

    QLabel* vbrHint = new QLabel(tr("提示: VBR质量 0=最佳质量(文件大), 9=最大压缩(质量低)"));
    vbrHint->setStyleSheet("color: #888; font-size: 11px;");
    bitrateGrid->addWidget(vbrHint, 4, 0, 1, 2);

    mainLayout->addWidget(bitrateGroup);

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

void AudioParamsWidget::setupConnections() {
    auto updatePreview = [this]() {
        m_previewLabel->setText(buildPreviewText());
        emit paramsChanged();
    };

    connect(m_codecCombo, &QComboBox::currentTextChanged, this, updatePreview);
    connect(m_bitrateCombo, &QComboBox::currentTextChanged, this, updatePreview);
    connect(m_sampleRateCombo, &QComboBox::currentTextChanged, this, updatePreview);
    connect(m_channelsCombo, &QComboBox::currentTextChanged, this, updatePreview);
    connect(m_vbrQualitySlider, &QSlider::valueChanged, m_vbrQualitySpinBox, &QSpinBox::setValue);
    connect(m_vbrQualitySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), m_vbrQualitySlider, &QSlider::setValue);
    connect(m_vbrQualitySlider, &QSlider::valueChanged, this, updatePreview);
}

QVariantMap AudioParamsWidget::getParams() const {
    QVariantMap params;
    params["audioCodec"] = m_codecCombo->currentData().toString();
    QString bitrate = m_bitrateCombo->currentData().toString();
    if (bitrate != "auto") {
        params["audioBitrate"] = bitrate;
    }
    int sampleRate = m_sampleRateCombo->currentData().toInt();
    if (sampleRate > 0) {
        params["sampleRate"] = sampleRate;
    }
    int channels = m_channelsCombo->currentData().toInt();
    if (channels > 0) {
        params["channels"] = channels;
    }
    params["vbrQuality"] = m_vbrQualitySpinBox->value();
    return params;
}

void AudioParamsWidget::setParams(const QVariantMap& params) {
    m_codecCombo->blockSignals(true);
    m_bitrateCombo->blockSignals(true);
    m_sampleRateCombo->blockSignals(true);
    m_channelsCombo->blockSignals(true);
    m_vbrQualitySlider->blockSignals(true);
    m_vbrQualitySpinBox->blockSignals(true);

    auto setComboData = [](QComboBox* combo, const QVariant& data) {
        int idx = combo->findData(data);
        if (idx >= 0) combo->setCurrentIndex(idx);
    };

    setComboData(m_codecCombo, params.value("audioCodec", "aac"));

    QString bitrate = params.value("audioBitrate").toString();
    if (!bitrate.isEmpty()) {
        setComboData(m_bitrateCombo, bitrate);
    }

    int sampleRate = params.value("sampleRate", 0).toInt();
    if (sampleRate > 0) {
        setComboData(m_sampleRateCombo, sampleRate);
    }

    int channels = params.value("channels", 0).toInt();
    if (channels > 0) {
        setComboData(m_channelsCombo, channels);
    }

    m_vbrQualitySpinBox->setValue(params.value("vbrQuality", 5).toInt());

    m_codecCombo->blockSignals(false);
    m_bitrateCombo->blockSignals(false);
    m_sampleRateCombo->blockSignals(false);
    m_channelsCombo->blockSignals(false);
    m_vbrQualitySlider->blockSignals(false);
    m_vbrQualitySpinBox->blockSignals(false);

    m_previewLabel->setText(buildPreviewText());
}

QStringList AudioParamsWidget::validate() const {
    QStringList errors;
    int vbr = m_vbrQualitySpinBox->value();
    if (vbr < 0 || vbr > 9) {
        errors << tr("VBR质量必须在 0-9 之间");
    }
    return errors;
}

QString AudioParamsWidget::buildPreviewText() const {
    QString text = tr("📋 音频参数预览\n");
    text += tr("━━━━━━━━━━━━━━━━━━\n");
    text += tr("编码器: %1\n").arg(m_codecCombo->currentText().section("(", 0, 0).trimmed());
    text += tr("比特率: %1\n").arg(m_bitrateCombo->currentText());
    text += tr("采样率: %1\n").arg(m_sampleRateCombo->currentText());
    text += tr("声道: %1\n").arg(m_channelsCombo->currentText());
    text += tr("VBR质量: %1 (0=最佳, 9=最大压缩)\n").arg(m_vbrQualitySpinBox->value());
    return text;
}
