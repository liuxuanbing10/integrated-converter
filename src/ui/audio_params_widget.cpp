#include "audio_params_widget.h"

#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>

namespace
{
template<typename T>
AbstractParamsWidget::Choice ch(const QString& text, const T& data)
{
    return qMakePair(text, QVariant::fromValue(data));
}
} // namespace

AudioParamsWidget::AudioParamsWidget(QWidget* parent) : AbstractParamsWidget(parent)
{
    // ── Codec ─────────────────────────────────────────────────────
    {
        QGridLayout* g = addGroup(tr("编码器"));
        m_codecCombo = addCombo(g, 0, tr("编码器:"),
                                {
                                    ch("AAC (Advanced Audio Coding)", QStringLiteral("aac")),
                                    ch("MP3 (MPEG Audio Layer 3)", QStringLiteral("mp3")),
                                    ch("FLAC (Free Lossless Audio Codec)", QStringLiteral("flac")),
                                    ch("Vorbis (OGG)", QStringLiteral("vorbis")),
                                    ch("Opus", QStringLiteral("opus")),
                                    ch("WAV (PCM)", QStringLiteral("pcm_s16le")),
                                },
                                32);
    }

    // ── Bitrate / sample rate / channels / VBR ────────────────────
    {
        QGridLayout* g = addGroup(tr("比特率设置"));
        m_bitrateCombo = addCombo(g, 0, tr("比特率:"),
                                  {
                                      ch(tr("自动"), QStringLiteral("auto")),
                                      ch("64 kbps", QStringLiteral("64k")),
                                      ch("96 kbps", QStringLiteral("96k")),
                                      ch("128 kbps", QStringLiteral("128k")),
                                      ch("192 kbps", QStringLiteral("192k")),
                                      ch("256 kbps", QStringLiteral("256k")),
                                      ch("320 kbps", QStringLiteral("320k")),
                                  });
        m_sampleRateCombo = addCombo(g, 1, tr("采样率:"),
                                     {
                                         ch(tr("自动"), 0),
                                         ch("22050 Hz", 22050),
                                         ch("44100 Hz", 44100),
                                         ch("48000 Hz", 48000),
                                         ch("96000 Hz", 96000),
                                         ch("192000 Hz", 192000),
                                     });
        m_channelsCombo = addCombo(g, 2, tr("声道:"),
                                   {
                                       ch(tr("自动"), 0),
                                       ch(tr("单声道 (Mono)"), 1),
                                       ch(tr("立体声 (Stereo)"), 2),
                                       ch(tr("环绕声 5.1"), 6),
                                   });

        QHBoxLayout* vbrRow = new QHBoxLayout();
        vbrRow->addWidget(new QLabel(tr("VBR质量:")));
        m_vbrQualitySlider = new QSlider(Qt::Horizontal);
        m_vbrQualitySlider->setRange(0, 9);
        m_vbrQualitySlider->setValue(5);
        m_vbrQualitySlider->setTickPosition(QSlider::TicksBelow);
        m_vbrQualitySlider->setTickInterval(1);
        vbrRow->addWidget(m_vbrQualitySlider, 1);
        m_vbrQualitySpinBox = new QSpinBox();
        m_vbrQualitySpinBox->setRange(0, 9);
        m_vbrQualitySpinBox->setValue(5);
        m_vbrQualitySpinBox->setFixedWidth(70);
        m_vbrQualitySpinBox->setToolTip(tr("0=最高质量, 9=最高压缩"));
        vbrRow->addWidget(m_vbrQualitySpinBox);
        g->addLayout(vbrRow, 3, 0, 1, 2);
        syncPair(m_vbrQualitySlider, m_vbrQualitySpinBox);
        registerControl(m_vbrQualitySlider, SIGNAL(valueChanged(int)));

        addHint(g, 4, tr("提示: VBR质量 0=最佳质量(文件大), 9=最大压缩(质量低)"));
    }

    finishSetup();
}

QVariantMap AudioParamsWidget::collectParams() const
{
    QVariantMap params;
    params["audioCodec"] = m_codecCombo->currentData().toString();
    const QString bitrate = m_bitrateCombo->currentData().toString();
    if (bitrate != "auto")
    {
        params["audioBitrate"] = bitrate;
    }
    const int sampleRate = m_sampleRateCombo->currentData().toInt();
    if (sampleRate > 0)
    {
        params["sampleRate"] = sampleRate;
    }
    const int channels = m_channelsCombo->currentData().toInt();
    if (channels > 0)
    {
        params["channels"] = channels;
    }
    params["vbrQuality"] = m_vbrQualitySpinBox->value();
    return params;
}

void AudioParamsWidget::applyParams(const QVariantMap& params)
{
    setComboData(m_codecCombo, params.value("audioCodec", "aac"));

    const QString bitrate = params.value("audioBitrate").toString();
    if (!bitrate.isEmpty())
    {
        setComboData(m_bitrateCombo, bitrate);
    }

    const int sampleRate = params.value("sampleRate", 0).toInt();
    if (sampleRate > 0)
    {
        setComboData(m_sampleRateCombo, sampleRate);
    }

    const int channels = params.value("channels", 0).toInt();
    if (channels > 0)
    {
        setComboData(m_channelsCombo, channels);
    }

    const int vbr = params.value("vbrQuality", 5).toInt();
    m_vbrQualitySpinBox->setValue(vbr);
    m_vbrQualitySlider->setValue(vbr);
}

QStringList AudioParamsWidget::validate() const
{
    QStringList errors;
    const int vbr = m_vbrQualitySpinBox->value();
    if (vbr < 0 || vbr > 9)
    {
        errors << tr("VBR质量必须在 0-9 之间");
    }
    return errors;
}

QString AudioParamsWidget::buildPreviewText() const
{
    QString text = tr("📋 音频参数预览\n");
    text += separatorLine();
    text += tr("编码器: %1\n").arg(m_codecCombo->currentText().section("(", 0, 0).trimmed());
    text += tr("比特率: %1\n").arg(m_bitrateCombo->currentText());
    text += tr("采样率: %1\n").arg(m_sampleRateCombo->currentText());
    text += tr("声道: %1\n").arg(m_channelsCombo->currentText());
    text += tr("VBR质量: %1 (0=最佳, 9=最大压缩)\n").arg(m_vbrQualitySpinBox->value());
    return text;
}
