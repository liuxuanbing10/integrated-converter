#include "task_list_widget.h"
#include "conversion_task.h"
#include "task_manager.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QDesktopServices>
#include <QFileInfo>
#include <QDir>
#include <QUrl>

TaskListWidget::TaskListWidget(QWidget* parent)
    : QWidget(parent)
    , m_tableWidget(nullptr)
    , m_infoLabel(nullptr)
    , m_startButton(nullptr)
    , m_cancelButton(nullptr)
    , m_removeButton(nullptr)
{
    setupUI();
    setupConnections();
}

TaskListWidget::~TaskListWidget() {
    qDeleteAll(m_progressBars);
    m_progressBars.clear();
}

void TaskListWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(6);
    m_tableWidget->setHorizontalHeaderLabels({
        tr("文件名"), tr("状态"), tr("进度"), tr("转换器"), tr("输出格式"), tr("操作")
    });
    m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    m_tableWidget->setColumnWidth(0, 200);
    m_tableWidget->setColumnWidth(1, 80);
    m_tableWidget->setColumnWidth(2, 120);
    m_tableWidget->setColumnWidth(3, 80);
    m_tableWidget->setColumnWidth(4, 80);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setAlternatingRowColors(true);
    m_tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tableWidget->verticalHeader()->setDefaultSectionSize(36);
    m_tableWidget->setStyleSheet(
        "QTableWidget {"
        "  border: 1px solid #ccc;"
        "  border-radius: 4px;"
        "  gridline-color: #e0e0e0;"
        "}"
        "QTableWidget::item { padding: 4px; }"
        "QTableWidget::item:selected { background-color: #e3f2fd; color: #000; }"
        "QHeaderView::section {"
        "  background-color: #f5f5f5;"
        "  border: none;"
        "  border-bottom: 1px solid #ccc;"
        "  padding: 6px;"
        "  font-weight: bold;"
        "}"
    );
    mainLayout->addWidget(m_tableWidget);

    setStyleSheet(
        "QPushButton {"
        "  padding: 6px 12px;"
        "  border: 1px solid #ccc;"
        "  border-radius: 4px;"
        "  background-color: #fff;"
        "}"
        "QPushButton:hover { background-color: #f0f0f0; }"
        "QPushButton:pressed { background-color: #e0e0e0; }"
    );
}

void TaskListWidget::setupConnections() {
    connect(m_tableWidget, &QTableWidget::customContextMenuRequested,
            this, &TaskListWidget::onCustomContextMenu);
}

void TaskListWidget::refreshTaskList() {
    qDeleteAll(m_progressBars);
    m_progressBars.clear();
    m_tableWidget->setRowCount(0);
    QList<ConversionTask*> tasks = TaskManager::instance()->getAllTasks();
    m_tableWidget->setRowCount(tasks.count());
    for (int i = 0; i < tasks.count(); ++i) {
        updateTableRow(i, tasks[i]);
    }
}

void TaskListWidget::updateTableRow(int row, ConversionTask* task) {
    if (!task) return;
    QFileInfo fi(task->inputFile());
    QTableWidgetItem* nameItem = new QTableWidgetItem(fi.fileName());
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
    progressBar->setStyleSheet(
        "QProgressBar {"
        "  border: 1px solid #ccc;"
        "  border-radius: 3px;"
        "  text-align: center;"
        "  background-color: #f5f5f5;"
        "}"
        "QProgressBar::chunk { background-color: #4CAF50; border-radius: 2px; }"
    );
    m_progressBars[taskId] = progressBar;
    m_tableWidget->setCellWidget(row, 2, progressBar);

    m_tableWidget->setItem(row, 3, new QTableWidgetItem(
        ConversionTask::converterTypeToString(task->converterType())
    ));

    QString outputFormat = task->params().value("outputFormat").toString();
    if (outputFormat.isEmpty()) {
        QFileInfo ofi(task->outputFile());
        outputFormat = ofi.suffix().toUpper();
    }
    m_tableWidget->setItem(row, 4, new QTableWidgetItem(outputFormat));

    QWidget* actionWidget = new QWidget();
    QHBoxLayout* actionLayout = new QHBoxLayout(actionWidget);
    actionLayout->setContentsMargins(2, 2, 2, 2);
    actionLayout->setSpacing(2);

    QPushButton* cancelBtn = new QPushButton(tr("取消"), this);
    cancelBtn->setProperty("taskId", taskId);
    cancelBtn->setMaximumWidth(50);
    cancelBtn->setStyleSheet(
        "QPushButton { padding: 2px 6px; font-size: 11px; }"
    );
    connect(cancelBtn, &QPushButton::clicked, [this, taskId]() {
        TaskManager::instance()->cancelTask(taskId);
        refreshTaskList();
    });
    actionLayout->addWidget(cancelBtn);

    m_tableWidget->setCellWidget(row, 5, actionWidget);
}

void TaskListWidget::updateTaskProgress(const QString& taskId, int progress) {
    int row = findTaskRow(taskId);
    if (row >= 0) {
        QProgressBar* bar = m_progressBars.value(taskId);
        if (bar) {
            bar->setValue(progress);
            bar->setFormat(QString("%1%").arg(progress));
        }
    }
}

void TaskListWidget::updateTaskStatus(const QString& taskId, int status) {
    int row = findTaskRow(taskId);
    if (row >= 0) {
        ConversionTask* task = TaskManager::instance()->getTask(taskId);
        if (task) {
            QTableWidgetItem* statusItem = m_tableWidget->item(row, 1);
            if (statusItem) {
                statusItem->setText(formatStatus(static_cast<ConversionTask::Status>(status)));
                statusItem->setForeground(statusColor(static_cast<ConversionTask::Status>(status)));
            }
        }
    }
}

int TaskListWidget::findTaskRow(const QString& taskId) const {
    for (int i = 0; i < m_tableWidget->rowCount(); ++i) {
        QTableWidgetItem* item = m_tableWidget->item(i, 0);
        if (item && item->data(Qt::UserRole).toString() == taskId) {
            return i;
        }
    }
    return -1;
}

QString TaskListWidget::formatStatus(ConversionTask::Status status) const {
    switch (status) {
        case ConversionTask::Status::Pending:   return tr("等待中");
        case ConversionTask::Status::Running:   return tr("转换中");
        case ConversionTask::Status::Completed: return tr("已完成");
        case ConversionTask::Status::Failed:    return tr("失败");
        case ConversionTask::Status::Cancelled: return tr("已取消");
        default:                                return tr("未知");
    }
}

QColor TaskListWidget::statusColor(ConversionTask::Status status) const {
    switch (status) {
        case ConversionTask::Status::Pending:   return QColor(0x7B, 0x1F, 0xA2);
        case ConversionTask::Status::Running:   return QColor(0xE6, 0x51, 0x00);
        case ConversionTask::Status::Completed: return QColor(0x2E, 0x7D, 0x32);
        case ConversionTask::Status::Failed:    return QColor(0xC6, 0x28, 0x28);
        case ConversionTask::Status::Cancelled: return QColor(0x45, 0x5A, 0x64);
        default:                                return QColor(0x00, 0x00, 0x00);
    }
}

void TaskListWidget::onStartSelected() {
    int row = m_tableWidget->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, tr("警告"), tr("请先选择一个任务"));
        return;
    }
    QTableWidgetItem* item = m_tableWidget->item(row, 0);
    if (item) {
        QString taskId = item->data(Qt::UserRole).toString();
        TaskManager::instance()->start();
    }
}

void TaskListWidget::onCancelSelected() {
    int row = m_tableWidget->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, tr("警告"), tr("请先选择一个任务"));
        return;
    }
    QTableWidgetItem* item = m_tableWidget->item(row, 0);
    if (item) {
        QString taskId = item->data(Qt::UserRole).toString();
        TaskManager::instance()->cancelTask(taskId);
        refreshTaskList();
    }
}

void TaskListWidget::onRemoveSelected() {
    int row = m_tableWidget->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, tr("警告"), tr("请先选择一个任务"));
        return;
    }
    QTableWidgetItem* item = m_tableWidget->item(row, 0);
    if (item) {
        QString taskId = item->data(Qt::UserRole).toString();
        TaskManager::instance()->removeTask(taskId);
        refreshTaskList();
    }
}

void TaskListWidget::onRetrySelected() {
    int row = m_tableWidget->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, tr("警告"), tr("请先选择一个任务"));
        return;
    }
    QTableWidgetItem* item = m_tableWidget->item(row, 0);
    if (item) {
        QString taskId = item->data(Qt::UserRole).toString();
        ConversionTask* task = TaskManager::instance()->getTask(taskId);
        if (task) {
            QString inputFile = task->inputFile();
            QString outputFile = task->outputFile();
            QVariantMap params = task->params();
            TaskManager::instance()->removeTask(taskId);
            TaskManager::instance()->addTask(inputFile, outputFile, params);
            refreshTaskList();
        }
    }
}

void TaskListWidget::onOpenOutputDirectory() {
    int row = m_tableWidget->currentRow();
    if (row < 0) return;
    QTableWidgetItem* item = m_tableWidget->item(row, 0);
    if (item) {
        QString taskId = item->data(Qt::UserRole).toString();
        ConversionTask* task = TaskManager::instance()->getTask(taskId);
        if (task) {
            QFileInfo fi(task->outputFile());
            QString dir = fi.absolutePath();
            QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
        }
    }
}

void TaskListWidget::onCustomContextMenu(const QPoint& pos) {
    QTableWidgetItem* item = m_tableWidget->itemAt(pos);
    if (!item) return;
    QMenu menu(this);
    menu.addAction(tr("取消"), this, &TaskListWidget::onCancelSelected);
    menu.addAction(tr("重试"), this, &TaskListWidget::onRetrySelected);
    menu.addAction(tr("移除"), this, &TaskListWidget::onRemoveSelected);
    menu.addSeparator();
    menu.addAction(tr("打开输出目录"), this, &TaskListWidget::onOpenOutputDirectory);
    menu.exec(m_tableWidget->mapToGlobal(pos));
}
