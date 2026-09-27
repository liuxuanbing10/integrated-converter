#include "document_params_widget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>

namespace
{
template<typename T>
AbstractParamsWidget::Choice ch(const QString& text, const T& data)
{
    return qMakePair(text, QVariant::fromValue(data));
}
} // namespace

DocumentParamsWidget::DocumentParamsWidget(QWidget* parent) : AbstractParamsWidget(parent)
{
    // ── Page layout ───────────────────────────────────────────────
    {
        QGridLayout* g = addGroup(tr("页面布局"));
        m_pageSizeCombo = addCombo(g, 0, tr("页面大小:"),
                                   {
                                       ch("A4 (210×297mm)", QStringLiteral("a4")),
                                       ch("A5 (148×210mm)", QStringLiteral("a5")),
                                       ch("Letter (216×279mm)", QStringLiteral("letter")),
                                       ch("Legal (216×356mm)", QStringLiteral("legal")),
                                       ch("Tabloid (279×432mm)", QStringLiteral("tabloid")),
                                   });
        m_orientationCombo = addCombo(g, 1, tr("方向:"),
                                      {
                                          ch(tr("纵向"), QStringLiteral("portrait")),
                                          ch(tr("横向"), QStringLiteral("landscape")),
                                      });
    }

    // ── Margins ───────────────────────────────────────────────────
    {
        QGridLayout* g = addGroup(tr("页边距 (英寸)"));
        auto makeMargin = [this]() {
            QDoubleSpinBox* sp = new QDoubleSpinBox();
            sp->setRange(0.0, 5.0);
            sp->setSingleStep(0.1);
            sp->setValue(1.0);
            sp->setSuffix(tr(" 英寸"));
            sp->setDecimals(1);
            sp->setMinimumHeight(30);
            return sp;
        };
        g->addWidget(new QLabel(tr("上:")), 0, 0);
        m_marginTop = makeMargin();
        g->addWidget(m_marginTop, 0, 1);
        registerControl(m_marginTop, SIGNAL(valueChanged(double)));
        g->addWidget(new QLabel(tr("下:")), 0, 2);
        m_marginBottom = makeMargin();
        g->addWidget(m_marginBottom, 0, 3);
        registerControl(m_marginBottom, SIGNAL(valueChanged(double)));
        g->addWidget(new QLabel(tr("左:")), 1, 0);
        m_marginLeft = makeMargin();
        g->addWidget(m_marginLeft, 1, 1);
        registerControl(m_marginLeft, SIGNAL(valueChanged(double)));
        g->addWidget(new QLabel(tr("右:")), 1, 2);
        m_marginRight = makeMargin();
        g->addWidget(m_marginRight, 1, 3);
        registerControl(m_marginRight, SIGNAL(valueChanged(double)));
    }

    // ── PDF options ───────────────────────────────────────────────
    {
        QGridLayout* g = addGroup(tr("PDF/输出选项"));
        m_pdfEngineCombo = addCombo(g, 0, tr("PDF引擎:"),
                                    {
                                        ch(tr("默认"), QString()),
                                        ch("pdflatex", QStringLiteral("pdflatex")),
                                        ch("xelatex", QStringLiteral("xelatex")),
                                        ch("lualatex", QStringLiteral("lualatex")),
                                        ch("wkhtmltopdf", QStringLiteral("wkhtmltopdf")),
                                        ch("weasyprint", QStringLiteral("weasyprint")),
                                    });
        m_tocCheckBox = new QCheckBox(tr("生成目录 (Table of Contents)"));
        m_tocCheckBox->setChecked(false);
        g->addWidget(m_tocCheckBox, 1, 0, 1, 2);
        registerControl(m_tocCheckBox, SIGNAL(toggled(bool)));

        g->addWidget(new QLabel(tr("目录深度:")), 2, 0);
        m_tocDepthSpinBox = new QSpinBox();
        m_tocDepthSpinBox->setRange(1, 6);
        m_tocDepthSpinBox->setValue(3);
        m_tocDepthSpinBox->setMinimumHeight(30);
        m_tocDepthSpinBox->setEnabled(false);
        g->addWidget(m_tocDepthSpinBox, 2, 1);
        registerControl(m_tocDepthSpinBox, SIGNAL(valueChanged(int)));

        m_numberSectionsCheckBox = new QCheckBox(tr("章节编号 (Number Sections)"));
        m_numberSectionsCheckBox->setChecked(false);
        g->addWidget(m_numberSectionsCheckBox, 3, 0, 1, 2);
        registerControl(m_numberSectionsCheckBox, SIGNAL(toggled(bool)));
    }

    finishSetup();
}

void DocumentParamsWidget::onControlChanged()
{
    // TOC depth only makes sense while the TOC itself is on.
    m_tocDepthSpinBox->setEnabled(m_tocCheckBox->isChecked());
}

QVariantMap DocumentParamsWidget::collectParams() const
{
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

void DocumentParamsWidget::applyParams(const QVariantMap& params)
{
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
}

QStringList DocumentParamsWidget::validate() const
{
    QStringList errors;
    if (m_marginTop->value() < 0 || m_marginBottom->value() < 0 || m_marginLeft->value() < 0 ||
        m_marginRight->value() < 0)
    {
        errors << tr("页边距不能为负数");
    }
    return errors;
}

QString DocumentParamsWidget::buildPreviewText() const
{
    QString text = tr("📋 文档参数预览\n");
    text += separatorLine();
    text += tr("页面大小: %1\n").arg(m_pageSizeCombo->currentText());
    text += tr("方向: %1\n").arg(m_orientationCombo->currentText());
    text += tr("边距: 上%1 / 下%2 / 左%3 / 右%4\n")
                .arg(QString::number(m_marginTop->value(), 'f', 1))
                .arg(QString::number(m_marginBottom->value(), 'f', 1))
                .arg(QString::number(m_marginLeft->value(), 'f', 1))
                .arg(QString::number(m_marginRight->value(), 'f', 1));
    if (!m_pdfEngineCombo->currentData().toString().isEmpty())
    {
        text += tr("PDF引擎: %1\n").arg(m_pdfEngineCombo->currentText());
    }
    if (m_tocCheckBox->isChecked())
    {
        text += tr("生成目录 (深度: %1)\n").arg(m_tocDepthSpinBox->value());
    }
    if (m_numberSectionsCheckBox->isChecked())
    {
        text += tr("章节编号: 是\n");
    }
    return text;
}
