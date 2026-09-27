#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include "conversion_task.h"
#include "iconverter.h"

#include <QList>
#include <QMap>
#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <QThreadPool>
#include <QWaitCondition>

#include <memory>

class ConversionTask;
class TaskRunnable;

class TaskManager : public QObject
{
    Q_OBJECT

public:
    static TaskManager* instance();

    void registerConverter(const QString& name, std::shared_ptr<IConverter> converter);
    void unregisterConverter(const QString& name);
    QStringList availableConverters() const;
    IConverter* converter(const QString& name) const;

    QString addTask(std::unique_ptr<ConversionTask> task);
    QString addTask(const QString& inputFile, const QString& outputFile, const QVariantMap& params);
    void removeTask(const QString& taskId);
    void cancelTask(const QString& taskId);
    void cancelAllTasks();

    ConversionTask* getTask(const QString& taskId) const;
    QList<ConversionTask*> getAllTasks() const;
    QList<ConversionTask*> getPendingTasks() const;
    QList<ConversionTask*> getRunningTasks() const;
    QList<ConversionTask*> getCompletedTasks() const;
    QList<ConversionTask*> getFailedTasks() const;

    void start();
    bool isRunning() const;
    // Pause = stop dispatching NEW tasks; already-running conversions are not
    // interrupted (3FUI queue semantics: ffmpeg processes are not resumable).
    void pause();
    void resume();
    bool isPaused() const;
    /// Blocks until all worker threads finish (after cooperative cancel).
    /// Call on app exit BEFORE releasing the event loop so queued
    /// onTaskFinished slots still run and orphaned tasks get deleted.
    void waitForTasks(int timeoutMs = 5000);

    void setMaxParallelTasks(int max);
    int maxParallelTasks() const;

    struct TaskCounters
    {
        int total = 0;
        int pending = 0;
        int running = 0;
        int completed = 0;
        int failed = 0;
    };
    TaskCounters counters() const;

    int totalTaskCount() const;
    int pendingCount() const;
    int runningCount() const;
    int completedCount() const;
    int failedCount() const;

signals:
    void taskAdded(const QString& taskId);
    void taskStarted(const QString& taskId);
    void taskProgressChanged(const QString& taskId, int progress);
    void taskCompleted(const QString& taskId, bool success);
    void taskRemoved(const QString& taskId);
    void allTasksCompleted();
    void pauseStateChanged(bool paused);

private slots:
    void onTaskStarted(const QString& taskId);
    void onTaskProgressChanged(const QString& taskId, int progress);
    void onTaskFinished(const QString& taskId, bool success, const QString& message);

private:
    TaskManager();
    ~TaskManager();
    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;

    void processQueue();
    void insertTaskByPriority(const QString& taskId);
    void updateTaskPriority(const QString& taskId);

    /// Core cancel logic — caller MUST already hold m_mutex.
    void cancelAllTasksInternal();

    QMap<QString, std::shared_ptr<IConverter>> m_converters;
    QMap<QString, ConversionTask*> m_tasks;
    QMap<QString, TaskRunnable*> m_runningTasks;
    // Tasks removed while still Running: the worker thread may still touch
    // them until TaskRunnable::run() returns. Ownership moves here and the
    // delete happens in onTaskFinished instead of removeTask (fixes the old
    // use-after-free where removeTask deleteLater()'d a live task).
    QMap<QString, ConversionTask*> m_orphanedTasks;
    QList<QString> m_pendingQueue;
    mutable QMutex m_mutex;
    QThreadPool* m_threadPool;
    int m_maxParallel;
    bool m_started;
    bool m_paused;
};
#endif // TASK_MANAGER_H
