#ifndef TASK_LIST_WIDGET_H
#define TASK_LIST_WIDGET_H
#include <QWidget>
#include <QTableWidget>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QLabel>
#include "conversion_task.h"
class TaskListWidget : public QWidget {
    Q_OBJECT
public:
    explicit TaskListWidget(QWidget* parent = nullptr);
    ~TaskListWidget() override;
public slots:
    void refreshTaskList();
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
    QTableWidget* m_tableWidget;
    QLabel* m_infoLabel;
    QPushButton* m_startButton;
    QPushButton* m_cancelButton;
    QPushButton* m_removeButton;
    QMap<QString, QProgressBar*> m_progressBars;
};
#endif // TASK_LIST_WIDGET_H
