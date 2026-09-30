#include "preset_chips.h"

#include "theme.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>

PresetChipsWidget::PresetChipsWidget(PresetLibrary* library, QWidget* parent) : QWidget(parent), m_library(library)
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(6);
    hide();
}

void PresetChipsWidget::showCategory(FormatRegistry::Category category)
{
    m_category = category;
    rebuild();
}

void PresetChipsWidget::rebuild()
{
    while (QLayoutItem* item = m_layout->takeAt(0))
    {
        if (QWidget* w = item->widget())
        {
            w->deleteLater();
        }
        delete item;
    }

    QList<Preset> matching;
    for (const Preset& p : m_library->all())
    {
        if (p.category == m_category)
        {
            matching << p;
        }
    }
    if (matching.isEmpty())
    {
        hide();
        return;
    }

    QLabel* hint = new QLabel(tr("预设:"));
    Theme::setCss(hint, "muted");
    m_layout->addWidget(hint);
    for (const Preset& p : matching)
    {
        QPushButton* chip = new QPushButton(p.name);
        chip->setCursor(Qt::PointingHandCursor);
        chip->setToolTip(tr("点击套用 · 右键导出/删除\n目标格式 .%1").arg(p.format));
        Theme::setCss(chip, "chip-btn");
        const Preset captured = p;
        connect(chip, &QPushButton::clicked, this, [this, captured]() { emit applyRequested(captured); });
        chip->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(chip, &QWidget::customContextMenuRequested, this, [this, chip, captured](const QPoint&) {
            QMenu menu(chip);
            QAction* exportAct = menu.addAction(tr("导出 .json 分享…"));
            QAction* removeAct = menu.addAction(tr("删除预设"));
            QAction* chosen = menu.exec(QCursor::pos());
            if (chosen == exportAct)
            {
                exportPreset(captured);
            }
            else if (chosen == removeAct)
            {
                deletePreset(captured);
            }
        });
        m_layout->addWidget(chip);
    }
    m_layout->addStretch();
    show();
}

void PresetChipsWidget::exportPreset(const Preset& preset)
{
    const QString suggested = QDir::homePath() + QStringLiteral("/%1.json").arg(preset.name);
    const QString target = QFileDialog::getSaveFileName(this, tr("导出预设"), suggested, tr("预设文件 (*.json)"));
    if (target.isEmpty())
    {
        return;
    }
    QString err;
    if (!m_library->exportTo(preset, target, &err))
    {
        QMessageBox::warning(this, tr("导出失败"), err);
    }
}

void PresetChipsWidget::deletePreset(const Preset& preset)
{
    if (QMessageBox::question(this, tr("删除预设"), tr("确定删除「%1」？").arg(preset.name),
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
    {
        return;
    }
    QString err;
    if (!m_library->remove(preset.id, &err))
    {
        QMessageBox::warning(this, tr("删除失败"), err);
        return;
    }
    rebuild();
}
