#ifndef TASK_LIST_WIDGET_H
#define TASK_LIST_WIDGET_H
#include "conversion_task.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>
class TaskListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TaskListWidget(QWidget* parent = nullptr);
    ~TaskListWidget() override;
signals:
    /// Emitted whenever the selected row changes (taskId, or empty when none).
    void selectionChanged(const QString& taskId);
    /// Emitted on double-click of a row — open output dir (3FUI locate action).
    void rowActivated(const QString& taskId);
public slots:
    void refreshTaskList();
    /// Task row currently selected, empty if none.
    QString selectedTaskId() const;
    void removeSelected();
    void updateTaskProgress(const QString& taskId, int progress);
    void updateTaskStatus(const QString& taskId, int status);
private slots:
    void onStartSelected();
    void onCancelSelected();
    void onRemoveSelected();
    void onRetrySelected();
    void onOpenOutputDirectory();
    void onCustomContextMenu(const QPoint& pos);

private:
    void setupUI();
    void setupConnections();
    void updateTableRow(int row, ConversionTask* task);
    int findTaskRow(const QString& taskId) const;
    QString formatStatus(ConversionTask::Status status) const;
    QColor statusColor(ConversionTask::Status status) const;
    QString formatEta(const ConversionTask* task) const;
    QString rowTaskId(int row) const;
    QTableWidget* m_tableWidget;
    QLabel* m_infoLabel;
    QPushButton* m_startButton;
    QPushButton* m_cancelButton;
    QPushButton* m_removeButton;
    QMap<QString, QProgressBar*> m_progressBars;
};
#endif // TASK_LIST_WIDGET_H
