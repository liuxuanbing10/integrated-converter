#include "task_list_widget.h"

#include "conversion_task.h"
#include "task_manager.h"
#include "theme.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QHeaderView>
#include <QMessageBox>
#include <QSizePolicy>
#include <QUrl>

TaskListWidget::TaskListWidget(QWidget* parent) :
    QWidget(parent),
    m_tableWidget(nullptr),
    m_infoLabel(nullptr),
    m_startButton(nullptr),
    m_cancelButton(nullptr),
    m_removeButton(nullptr)
{
    setupUI();
    setupConnections();
}

TaskListWidget::~TaskListWidget()
{
    qDeleteAll(m_progressBars);
    m_progressBars.clear();
}

void TaskListWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(7);
    m_tableWidget->setHorizontalHeaderLabels(
        {tr("任务名称"), tr("状态"), tr("进度"), tr("预计剩余"), tr("转换器"), tr("输出格式"), tr("操作")});
    m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);
    m_tableWidget->setColumnWidth(6, 70);
    m_tableWidget->horizontalHeader()->setStretchLastSection(false);
    m_tableWidget->setColumnWidth(1, 64);
    m_tableWidget->setColumnWidth(2, 120);
    m_tableWidget->setColumnWidth(3, 84);
    m_tableWidget->setColumnWidth(4, 78);
    m_tableWidget->setColumnWidth(5, 72);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setAlternatingRowColors(true);
    m_tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tableWidget->verticalHeader()->setDefaultSectionSize(38);
    if (auto* h0 = m_tableWidget->horizontalHeaderItem(0))
        h0->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    // Without min-width 0 the table's fixed column widths propagate up as a
    // huge minimumSizeHint and squeeze the config panel off the window.
    m_tableWidget->setMinimumWidth(0);
    m_tableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mainLayout->addWidget(m_tableWidget);
}

void TaskListWidget::setupConnections()
{
    connect(m_tableWidget, &QTableWidget::customContextMenuRequested, this, &TaskListWidget::onCustomContextMenu);
    connect(m_tableWidget, &QTableWidget::currentCellChanged, this,
            [this](int row, int, int, int) { emit selectionChanged(row >= 0 ? rowTaskId(row) : QString()); });
    connect(m_tableWidget, &QTableWidget::itemDoubleClicked, this, [this](QTableWidgetItem* item) {
        if (item)
        {
            const QString id = item->data(Qt::UserRole).toString();
            if (!id.isEmpty())
                emit rowActivated(id);
        }
    });
}

QString TaskListWidget::rowTaskId(int row) const
{
    QTableWidgetItem* item = m_tableWidget->item(row, 0);
    return item ? item->data(Qt::UserRole).toString() : QString();
}

QString TaskListWidget::selectedTaskId() const
{
    int row = m_tableWidget->currentRow();
    return row >= 0 ? rowTaskId(row) : QString();
}

void TaskListWidget::removeSelected()
{
    onRemoveSelected();
}

void TaskListWidget::refreshTaskList()
{
    qDeleteAll(m_progressBars);
    m_progressBars.clear();
    m_tableWidget->setRowCount(0);
    QList<ConversionTask*> tasks = TaskManager::instance()->getAllTasks();
    m_tableWidget->setRowCount(tasks.count());
    for (int i = 0; i < tasks.count(); ++i)
    {
        updateTableRow(i, tasks[i]);
    }
}

void TaskListWidget::updateTableRow(int row, ConversionTask* task)
{
    if (!task)
        return;
    QFileInfo fi(task->inputFile());
    QTableWidgetItem* nameItem = new QTableWidgetItem(fi.fileName());
    nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    nameItem->setToolTip(task->inputFile());
    nameItem->setData(Qt::UserRole, task->id());
    m_tableWidget->setItem(row, 0, nameItem);

    QTableWidgetItem* statusItem = new QTableWidgetItem(formatStatus(task->status()));
    statusItem->setForeground(statusColor(task->status()));
    m_tableWidget->setItem(row, 1, statusItem);

    QString taskId = task->id();
    QProgressBar* progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100);
    progressBar->setValue(task->progress());
    progressBar->setTextVisible(true);
    progressBar->setFormat(QString("%1%").arg(task->progress()));
    progressBar->setFixedHeight(18);
    m_progressBars[taskId] = progressBar;
    m_tableWidget->setCellWidget(row, 2, progressBar);

    QLabel* etaLabel = new QLabel(formatEta(task));
    Theme::setCss(etaLabel, "muted");
    etaLabel->setAlignment(Qt::AlignCenter);
    m_tableWidget->setCellWidget(row, 3, etaLabel);

    m_tableWidget->setItem(row, 4, new QTableWidgetItem(ConversionTask::converterTypeToString(task->converterType())));

    QString outputFormat = task->params().value("outputFormat").toString();
    if (outputFormat.isEmpty())
    {
        QFileInfo ofi(task->outputFile());
        outputFormat = ofi.suffix().toUpper();
    }
    m_tableWidget->setItem(row, 5, new QTableWidgetItem(outputFormat));

    QWidget* actionWidget = new QWidget();
    QHBoxLayout* actionLayout = new QHBoxLayout(actionWidget);
    actionLayout->setContentsMargins(2, 2, 2, 2);
    actionLayout->setSpacing(2);

    QPushButton* cancelBtn = new QPushButton(tr("停止"), this);
    cancelBtn->setProperty("taskId", taskId);
    cancelBtn->setFixedWidth(52);
    Theme::setCss(cancelBtn, "row-danger");
    connect(cancelBtn, &QPushButton::clicked, [this, taskId]() {
        TaskManager::instance()->cancelTask(taskId);
        refreshTaskList();
    });
    actionLayout->addWidget(cancelBtn);

    m_tableWidget->setCellWidget(row, 6, actionWidget);
}

void TaskListWidget::updateTaskProgress(const QString& taskId, int progress)
{
    int row = findTaskRow(taskId);
    if (row >= 0)
    {
        QProgressBar* bar = m_progressBars.value(taskId);
        if (bar)
        {
            bar->setValue(progress);
            bar->setFormat(QString("%1%").arg(progress));
        }
        // Refresh the ETA cell as progress advances (3FUI live remaining time).
        if (QLabel* eta = qobject_cast<QLabel*>(m_tableWidget->cellWidget(row, 3)))
        {
            ConversionTask* task = TaskManager::instance()->getTask(taskId);
            eta->setText(formatEta(task));
        }
    }
}

void TaskListWidget::updateTaskStatus(const QString& taskId, int status)
{
    int row = findTaskRow(taskId);
    if (row >= 0)
    {
        ConversionTask* task = TaskManager::instance()->getTask(taskId);
        if (task)
        {
            QTableWidgetItem* statusItem = m_tableWidget->item(row, 1);
            if (statusItem)
            {
                statusItem->setText(formatStatus(static_cast<ConversionTask::Status>(status)));
                statusItem->setForeground(statusColor(static_cast<ConversionTask::Status>(status)));
            }
            if (QLabel* eta = qobject_cast<QLabel*>(m_tableWidget->cellWidget(row, 3)))
            {
                eta->setText(formatEta(task));
            }
        }
    }
}

int TaskListWidget::findTaskRow(const QString& taskId) const
{
    for (int i = 0; i < m_tableWidget->rowCount(); ++i)
    {
        QTableWidgetItem* item = m_tableWidget->item(i, 0);
        if (item && item->data(Qt::UserRole).toString() == taskId)
        {
            return i;
        }
    }
    return -1;
}

QString TaskListWidget::formatEta(const ConversionTask* task) const
{
    if (!task)
        return QStringLiteral("--");
    if (task->status() == ConversionTask::Status::Completed)
    {
        qint64 ms = task->durationMs();
        return ms > 0 ? tr("用时 %1s").arg(ms / 1000.0, 0, 'f', 1) : tr("已完成");
    }
    if (task->status() != ConversionTask::Status::Running)
        return QStringLiteral("--");
    const int progress = task->progress();
    qint64 elapsedMs = task->startTime().isValid() ? task->startTime().msecsTo(QDateTime::currentDateTime()) : 0;
    if (progress > 2 && elapsedMs > 500)
    {
        qint64 remainMs = elapsedMs * (100 - progress) / progress;
        int totalSec = static_cast<int>(remainMs / 1000);
        if (totalSec >= 3600)
            return tr("剩余 %1:%2:%3")
                .arg(totalSec / 3600)
                .arg(totalSec / 60 % 60, 2, 10, QChar('0'))
                .arg(totalSec % 60, 2, 10, QChar('0'));
        return tr("剩余 %1:%2").arg(totalSec / 60, 2, 10, QChar('0')).arg(totalSec % 60, 2, 10, QChar('0'));
    }
    return tr("计算中…");
}

QString TaskListWidget::formatStatus(ConversionTask::Status status) const
{
    switch (status)
    {
        case ConversionTask::Status::Pending:
            return tr("等待中");
        case ConversionTask::Status::Running:
            return tr("转换中");
        case ConversionTask::Status::Completed:
            return tr("已完成");
        case ConversionTask::Status::Failed:
            return tr("失败");
        case ConversionTask::Status::Cancelled:
            return tr("已取消");
        default:
            return tr("未知");
    }
}

QColor TaskListWidget::statusColor(ConversionTask::Status status) const
{
    switch (status)
    {
        case ConversionTask::Status::Pending:
            return Theme::current().textMuted;
        case ConversionTask::Status::Running:
            return Theme::current().warning;
        case ConversionTask::Status::Completed:
            return Theme::current().success; // success
        case ConversionTask::Status::Failed:
            return Theme::current().danger; // danger
        case ConversionTask::Status::Cancelled:
            return Theme::current().textMuted; // muted
        default:
            return Theme::current().text;
    }
}

void TaskListWidget::onStartSelected()
{
    int row = m_tableWidget->currentRow();
    if (row < 0)
    {
        QMessageBox::warning(this, tr("警告"), tr("请先选择一个任务"));
        return;
    }
    QTableWidgetItem* item = m_tableWidget->item(row, 0);
    if (item)
    {
        QString taskId = item->data(Qt::UserRole).toString();
        TaskManager::instance()->start();
    }
}

void TaskListWidget::onCancelSelected()
{
    int row = m_tableWidget->currentRow();
    if (row < 0)
    {
        QMessageBox::warning(this, tr("警告"), tr("请先选择一个任务"));
        return;
    }
    QTableWidgetItem* item = m_tableWidget->item(row, 0);
    if (item)
    {
        QString taskId = item->data(Qt::UserRole).toString();
        TaskManager::instance()->cancelTask(taskId);
        refreshTaskList();
    }
}

void TaskListWidget::onRemoveSelected()
{
    int row = m_tableWidget->currentRow();
    if (row < 0)
    {
        QMessageBox::warning(this, tr("警告"), tr("请先选择一个任务"));
        return;
    }
    QTableWidgetItem* item = m_tableWidget->item(row, 0);
    if (item)
    {
        QString taskId = item->data(Qt::UserRole).toString();
        TaskManager::instance()->removeTask(taskId);
        refreshTaskList();
    }
}

void TaskListWidget::onRetrySelected()
{
    int row = m_tableWidget->currentRow();
    if (row < 0)
    {
        QMessageBox::warning(this, tr("警告"), tr("请先选择一个任务"));
        return;
    }
    QTableWidgetItem* item = m_tableWidget->item(row, 0);
    if (item)
    {
        QString taskId = item->data(Qt::UserRole).toString();
        ConversionTask* task = TaskManager::instance()->getTask(taskId);
        if (task)
        {
            QString inputFile = task->inputFile();
            QString outputFile = task->outputFile();
            QVariantMap params = task->params();
            TaskManager::instance()->removeTask(taskId);
            TaskManager::instance()->addTask(inputFile, outputFile, params);
            refreshTaskList();
        }
    }
}

void TaskListWidget::onOpenOutputDirectory()
{
    int row = m_tableWidget->currentRow();
    if (row < 0)
        return;
    QTableWidgetItem* item = m_tableWidget->item(row, 0);
    if (item)
    {
        QString taskId = item->data(Qt::UserRole).toString();
        ConversionTask* task = TaskManager::instance()->getTask(taskId);
        if (task)
        {
            QFileInfo fi(task->outputFile());
            QString dir = fi.absolutePath();
            QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
        }
    }
}

void TaskListWidget::onCustomContextMenu(const QPoint& pos)
{
    QTableWidgetItem* item = m_tableWidget->itemAt(pos);
    if (!item)
        return;
    QMenu menu(this);
    menu.addAction(tr("取消"), this, &TaskListWidget::onCancelSelected);
    menu.addAction(tr("重试"), this, &TaskListWidget::onRetrySelected);
    menu.addAction(tr("移除"), this, &TaskListWidget::onRemoveSelected);
    menu.addSeparator();
    menu.addAction(tr("打开输出目录"), this, &TaskListWidget::onOpenOutputDirectory);
    menu.exec(m_tableWidget->mapToGlobal(pos));
}
