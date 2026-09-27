#include "image_params_widget.h"

#include "theme.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
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

ImageParamsWidget::ImageParamsWidget(QWidget* parent) : AbstractParamsWidget(parent)
{
    // ── Quality ───────────────────────────────────────────────────
    {
        QGridLayout* g = addGroup(tr("质量设置"));
        QHBoxLayout* row = new QHBoxLayout();
        QLabel* lab = new QLabel(tr("质量:"));
        lab->setMinimumWidth(50);
        row->addWidget(lab);
        m_qualitySlider = new QSlider(Qt::Horizontal);
        m_qualitySlider->setRange(1, 100);
        m_qualitySlider->setValue(85);
        m_qualitySlider->setTickPosition(QSlider::TicksBelow);
        m_qualitySlider->setTickInterval(10);
        row->addWidget(m_qualitySlider, 1);
        m_qualitySpinBox = new QSpinBox();
        m_qualitySpinBox->setRange(1, 100);
        m_qualitySpinBox->setValue(85);
        m_qualitySpinBox->setFixedWidth(80);
        m_qualitySpinBox->setSuffix(tr("%"));
        row->addWidget(m_qualitySpinBox);
        g->addLayout(row, 0, 0, 1, 2);
        syncPair(m_qualitySlider, m_qualitySpinBox);
        registerControl(m_qualitySlider, SIGNAL(valueChanged(int)));
    }

    // ── Resolution / resize ───────────────────────────────────────
    {
        QGridLayout* g = addGroup(tr("分辨率 / 缩放"));
        QLabel* lab = new QLabel(tr("缩放:"));
        lab->setMinimumWidth(50);
        g->addWidget(lab, 0, 0);
        m_resizeInput = new QLineEdit();
        m_resizeInput->setPlaceholderText(tr("例如: 800x600, 50%, x1080"));
        m_resizeInput->setMaxLength(21);
        m_resizeInput->setMinimumHeight(32);
        QRegularExpression resizeRx(R"(^\d+[xX]\d+!?$|^\d+%$|^x\d+$|^\d+$|^$)");
        m_resizeInput->setValidator(new QRegularExpressionValidator(resizeRx, this));
        g->addWidget(m_resizeInput, 0, 1);
        registerControl(m_resizeInput, SIGNAL(textChanged(QString)));
        QLabel* hint = new QLabel(tr("留空则不缩放"));
        Theme::setCss(hint, "hint");
        g->addWidget(hint, 1, 1);
    }

    // ── Compression ───────────────────────────────────────────────
    {
        QGridLayout* g = addGroup(tr("压缩选项"));
        m_compressionCombo = addCombo(g, 0, tr("压缩方式:"),
                                      {
                                          ch(tr("无"), QString()),
                                          ch("JPEG", QStringLiteral("JPEG")),
                                          ch("LZW", QStringLiteral("LZW")),
                                          ch("RLE", QStringLiteral("RLE")),
                                          ch("Zip", QStringLiteral("Zip")),
                                      });
        g->addWidget(new QLabel(tr("DPI:")), 1, 0);
        m_densitySpinBox = new QSpinBox();
        m_densitySpinBox->setRange(0, 12000);
        m_densitySpinBox->setValue(0);
        m_densitySpinBox->setSuffix(tr(" dpi"));
        m_densitySpinBox->setSpecialValueText(tr("默认"));
        m_densitySpinBox->setMinimumHeight(30);
        g->addWidget(m_densitySpinBox, 1, 1);
        registerControl(m_densitySpinBox, SIGNAL(valueChanged(int)));
        m_depthCombo = addCombo(g, 2, tr("位深度:"),
                                {
                                    ch(tr("默认"), QString()),
                                    ch("8", QStringLiteral("8")),
                                    ch("16", QStringLiteral("16")),
                                    ch("32", QStringLiteral("32")),
                                });
    }

    m_stripCheckBox = new QCheckBox(tr("清理元数据 (Strip) — 移除EXIF等信息以减小文件体积"));
    m_body->addWidget(m_stripCheckBox);
    registerControl(m_stripCheckBox, SIGNAL(toggled(bool)));

    finishSetup();
}

void ImageParamsWidget::setEnabledFormats(const QStringList& formats)
{
    Q_UNUSED(formats);
    // Format selection is handled by the main window's config panel.
    // This widget focuses on conversion parameters only.
    repaintPreview();
}

QVariantMap ImageParamsWidget::collectParams() const
{
    QVariantMap params;
    params["quality"] = m_qualitySpinBox->value();
    const QString resizeText = m_resizeInput->text().trimmed();
    if (!resizeText.isEmpty())
    {
        params["resize"] = resizeText;
    }
    params["compression"] = m_compressionCombo->currentData().toString();
    const int density = m_densitySpinBox->value();
    if (density > 0)
    {
        params["density"] = density;
    }
    params["strip"] = m_stripCheckBox->isChecked();
    params["depth"] = m_depthCombo->currentData().toString();
    return params;
}

void ImageParamsWidget::applyParams(const QVariantMap& params)
{
    const int quality = params.value("quality", 85).toInt();
    m_qualitySpinBox->setValue(quality);
    m_qualitySlider->setValue(quality);
    m_resizeInput->setText(params.value("resize").toString());
    setComboData(m_compressionCombo, params.value("compression"));
    m_densitySpinBox->setValue(params.value("density", 0).toInt());
    m_stripCheckBox->setChecked(params.value("strip", false).toBool());
    setComboData(m_depthCombo, params.value("depth"));
}

QStringList ImageParamsWidget::validate() const
{
    QStringList errors;
    const int quality = m_qualitySpinBox->value();
    if (quality < 1 || quality > 100)
    {
        errors << tr("质量值必须在 1-100 之间 (当前: %1)").arg(quality);
    }
    const QString resize = m_resizeInput->text().trimmed();
    if (!resize.isEmpty())
    {
        static const QRegularExpression resizeRe(R"(^\d+[xX]\d+!?$|^\d+%$|^x\d+$|^\d+$)");
        if (!resizeRe.match(resize).hasMatch())
        {
            errors << tr("缩放格式无效: \"%1\" (应为 800x600, 50%, x1080 或 800)").arg(resize);
        }
    }
    const int density = m_densitySpinBox->value();
    if (density < 0 || density > 12000)
    {
        errors << tr("DPI 超出范围 (0-12000)");
    }
    return errors;
}

QString ImageParamsWidget::buildPreviewText() const
{
    QString text = tr("📋 图片参数预览\n");
    text += separatorLine();
    text += tr("质量: %1%\n").arg(m_qualitySpinBox->value());
    const QString resize = m_resizeInput->text().trimmed();
    if (!resize.isEmpty())
    {
        text += tr("缩放: %1\n").arg(resize);
    }
    const QString comp = m_compressionCombo->currentText();
    if (comp != tr("无"))
    {
        text += tr("压缩: %1\n").arg(comp);
    }
    if (m_densitySpinBox->value() > 0)
    {
        text += tr("DPI: %1\n").arg(m_densitySpinBox->value());
    }
    if (m_stripCheckBox->isChecked())
    {
        text += tr("清理元数据: 是\n");
    }
    const QString depth = m_depthCombo->currentText();
    if (depth != tr("默认"))
    {
        text += tr("位深度: %1\n").arg(depth);
    }
    return text;
}
