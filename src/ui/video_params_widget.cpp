#include "video_params_widget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
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

VideoParamsWidget::VideoParamsWidget(QWidget* parent) : AbstractParamsWidget(parent)
{
    // ── Codecs ────────────────────────────────────────────────────
    {
        QGridLayout* g = addGroup(tr("编码器选择"));
        m_videoCodecCombo = addCombo(g, 0, tr("视频编码器:"),
                                     {
                                         ch("H.264 (AVC)", QStringLiteral("h264")),
                                         ch("H.265 (HEVC)", QStringLiteral("hevc")),
                                         ch("VP9", QStringLiteral("vp9")),
                                         ch("AV1", QStringLiteral("av1")),
                                         ch("MPEG-4", QStringLiteral("mpeg4")),
                                         ch(tr("自动"), QStringLiteral("auto")),
                                     },
                                     32);
        m_audioCodecCombo = addCombo(g, 1, tr("音频编码器:"),
                                     {
                                         ch("AAC", QStringLiteral("aac")),
                                         ch("MP3", QStringLiteral("mp3")),
                                         ch(tr("复制源音频"), QStringLiteral("copy")),
                                         ch(tr("无音频"), QStringLiteral("none")),
                                     });
    }

    // ── Quality / CRF ─────────────────────────────────────────────
    {
        QGridLayout* g = addGroup(tr("质量 / CRF设置"));
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
        g->addLayout(crfRow, 0, 0, 1, 2);
        syncPair(m_crfSlider, m_crfSpinBox);
        registerControl(m_crfSlider, SIGNAL(valueChanged(int)));
        addHint(g, 1, tr("提示: 0=无损, 23=默认, 51=最差质量/最小文件"));
    }

    // ── Resolution & Bitrate ──────────────────────────────────────
    {
        QGridLayout* g = addGroup(tr("分辨率与码率"));
        g->addWidget(new QLabel(tr("分辨率:")), 0, 0);
        m_resolutionInput = new QLineEdit();
        m_resolutionInput->setPlaceholderText(tr("例如: 1920x1080, 1280x720, 留空=原始"));
        m_resolutionInput->setMaxLength(21);
        m_resolutionInput->setMinimumHeight(30);
        QRegularExpression resRx(R"(^\d+[xX]\d+$|^$)");
        m_resolutionInput->setValidator(new QRegularExpressionValidator(resRx, this));
        g->addWidget(m_resolutionInput, 0, 1);
        registerControl(m_resolutionInput, SIGNAL(textChanged(QString)));

        m_videoBitrateCombo = addCombo(g, 1, tr("视频码率:"),
                                       {
                                           ch(tr("自动"), QString()),
                                           ch("500 kbps", QStringLiteral("500k")),
                                           ch("1 Mbps", QStringLiteral("1M")),
                                           ch("2 Mbps", QStringLiteral("2M")),
                                           ch("5 Mbps", QStringLiteral("5M")),
                                           ch("10 Mbps", QStringLiteral("10M")),
                                           ch("20 Mbps", QStringLiteral("20M")),
                                           ch("50 Mbps", QStringLiteral("50M")),
                                       });
        m_audioBitrateCombo = addCombo(g, 2, tr("音频码率:"),
                                       {
                                           ch(tr("自动"), QString()),
                                           ch("64 kbps", QStringLiteral("64k")),
                                           ch("128 kbps", QStringLiteral("128k")),
                                           ch("192 kbps", QStringLiteral("192k")),
                                           ch("256 kbps", QStringLiteral("256k")),
                                           ch("320 kbps", QStringLiteral("320k")),
                                       });
        m_framerateCombo = addCombo(g, 3, tr("帧率:"),
                                    {
                                        ch(tr("原始"), QString()),
                                        ch("24 fps", QStringLiteral("24")),
                                        ch("25 fps", QStringLiteral("25")),
                                        ch("30 fps", QStringLiteral("30")),
                                        ch("48 fps", QStringLiteral("48")),
                                        ch("60 fps", QStringLiteral("60")),
                                    });
    }

    // ── Preset & Advanced ─────────────────────────────────────────
    {
        QGridLayout* g = addGroup(tr("高级选项"));
        m_presetCombo = addCombo(g, 0, tr("编码预设:"),
                                 {
                                     ch("ultrafast", QStringLiteral("ultrafast")),
                                     ch("superfast", QStringLiteral("superfast")),
                                     ch("veryfast", QStringLiteral("veryfast")),
                                     ch("faster", QStringLiteral("faster")),
                                     ch("fast", QStringLiteral("fast")),
                                     ch("medium", QStringLiteral("medium")),
                                     ch("slow", QStringLiteral("slow")),
                                     ch("slower", QStringLiteral("slower")),
                                     ch("veryslow", QStringLiteral("veryslow")),
                                 });
        m_presetCombo->setCurrentText("medium");
        m_twoPassCheckBox = new QCheckBox(tr("二遍编码 (2-Pass) — 提高码率分配精度"));
        g->addWidget(m_twoPassCheckBox, 1, 0, 1, 2);
        registerControl(m_twoPassCheckBox, SIGNAL(toggled(bool)));
        addHint(g, 2, tr("提示: ultrafast=编码快但文件大, veryslow=编码慢但文件小"));
    }

    finishSetup();
}

QVariantMap VideoParamsWidget::collectParams() const
{
    QVariantMap params;
    params["videoCodec"] = m_videoCodecCombo->currentData().toString();
    params["audioCodec"] = m_audioCodecCombo->currentData().toString();
    const QString res = m_resolutionInput->text().trimmed();
    if (!res.isEmpty())
    {
        params["resolution"] = res;
    }
    const QString vbitrate = m_videoBitrateCombo->currentData().toString();
    if (!vbitrate.isEmpty())
    {
        params["videoBitrate"] = vbitrate;
    }
    const QString abitrate = m_audioBitrateCombo->currentData().toString();
    if (!abitrate.isEmpty())
    {
        params["audioBitrate"] = abitrate;
    }
    const QString fps = m_framerateCombo->currentData().toString();
    if (!fps.isEmpty())
    {
        params["framerate"] = fps;
    }
    params["preset"] = m_presetCombo->currentData().toString();
    params["crf"] = m_crfSpinBox->value();
    params["twoPass"] = m_twoPassCheckBox->isChecked();
    return params;
}

void VideoParamsWidget::applyParams(const QVariantMap& params)
{
    setComboData(m_videoCodecCombo, params.value("videoCodec", "h264"));
    setComboData(m_audioCodecCombo, params.value("audioCodec", "aac"));
    m_resolutionInput->setText(params.value("resolution").toString());
    setComboData(m_videoBitrateCombo, params.value("videoBitrate", ""));
    setComboData(m_audioBitrateCombo, params.value("audioBitrate", ""));
    setComboData(m_framerateCombo, params.value("framerate", ""));
    setComboData(m_presetCombo, params.value("preset", "medium"));
    const int crf = params.value("crf", 23).toInt();
    m_crfSpinBox->setValue(crf);
    m_crfSlider->setValue(crf);
    m_twoPassCheckBox->setChecked(params.value("twoPass", false).toBool());
}

QStringList VideoParamsWidget::validate() const
{
    QStringList errors;
    const int crf = m_crfSpinBox->value();
    if (crf < 0 || crf > 51)
    {
        errors << tr("CRF值必须在 0-51 之间 (当前: %1)").arg(crf);
    }
    const QString res = m_resolutionInput->text().trimmed();
    if (!res.isEmpty())
    {
        const QStringList parts = res.split(QRegularExpression("([xX])"));
        if (parts.size() == 2)
        {
            bool ok1, ok2;
            const int w = parts[0].toInt(&ok1);
            const int h = parts[1].toInt(&ok2);
            if (!ok1 || !ok2 || w <= 0 || h <= 0)
            {
                errors << tr("分辨率格式无效: \"%1\" (应为 WIDTHxHEIGHT, 如 1920x1080)").arg(res);
            }
        }
        else
        {
            errors << tr("分辨率格式无效: \"%1\"").arg(res);
        }
    }
    return errors;
}

QString VideoParamsWidget::buildPreviewText() const
{
    QString text = tr("📋 视频参数预览\n");
    text += separatorLine();
    text += tr("视频编码器: %1\n").arg(m_videoCodecCombo->currentText());
    text += tr("音频编码器: %1\n").arg(m_audioCodecCombo->currentText());
    text += tr("CRF: %1 (0=无损, 51=最差)\n").arg(m_crfSpinBox->value());
    const QString res = m_resolutionInput->text().trimmed();
    if (!res.isEmpty())
    {
        text += tr("分辨率: %1\n").arg(res);
    }
    if (!m_videoBitrateCombo->currentData().toString().isEmpty())
    {
        text += tr("视频码率: %1\n").arg(m_videoBitrateCombo->currentText());
    }
    if (!m_audioBitrateCombo->currentData().toString().isEmpty())
    {
        text += tr("音频码率: %1\n").arg(m_audioBitrateCombo->currentText());
    }
    if (!m_framerateCombo->currentData().toString().isEmpty())
    {
        text += tr("帧率: %1\n").arg(m_framerateCombo->currentText());
    }
    text += tr("预设: %1\n").arg(m_presetCombo->currentText());
    if (m_twoPassCheckBox->isChecked())
    {
        text += tr("二遍编码: 是\n");
    }
    return text;
}
