#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QMutex>
#include <QMutexLocker>
#include <QThreadPool>
#include <QWaitCondition>
#include <memory>
#include "conversion_task.h"
#include "iconverter.h"

class TaskRunnable;

class TaskManager : public QObject {
    Q_OBJECT

public:
    static TaskManager* instance();

    void registerConverter(const QString& name, std::shared_ptr<IConverter> converter);
    void unregisterConverter(const QString& name);
    QStringList availableConverters() const;
    IConverter* converter(const QString& name) const;

    QString addTask(ConversionTask* task);
    QString addTask(const QString& inputFile, const QString& outputFile,
                    const QVariantMap& params);
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
    void pause();
    void resume();
    bool isPaused() const;
    bool isRunning() const;

    void setMaxParallelTasks(int max);
    int maxParallelTasks() const;

    int totalTaskCount() const;
    int pendingCount() const;
    int runningCount() const;
    int completedCount() const;
    int failedCount() const;

    bool isMemoryUnderPressure() const;
    void setMemoryPressureThreshold(double ratio);
    void adjustParallelTasksForMemory();

signals:
    void taskAdded(const QString& taskId);
    void taskStarted(const QString& taskId);
    void taskProgressChanged(const QString& taskId, int progress);
    void taskCompleted(const QString& taskId, bool success);
    void taskRemoved(const QString& taskId);
    void allTasksCompleted();
    void memoryPressureChanged(bool underPressure);

private slots:
    void onTaskStarted(const QString& taskId);
    void onTaskProgressChanged(const QString& taskId, int progress);
    void onTaskFinished(const QString& taskId, bool success, const QString& message);
    void onMemoryWarning(size_t current, size_t threshold);
    void onMemoryCritical(size_t current, size_t threshold);
    void onMemoryNormalized();

private:
    TaskManager();
    ~TaskManager();
    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;

    void processQueue();
    void insertTaskByPriority(const QString& taskId);
    void updateTaskPriority(const QString& taskId);
    int calculateDynamicMaxParallel() const;

    QMap<QString, std::shared_ptr<IConverter>> m_converters;
    QMap<QString, ConversionTask*> m_tasks;
    QMap<QString, TaskRunnable*> m_runningTasks;
    QList<QString> m_pendingQueue;
    mutable QMutex m_mutex;
    QThreadPool* m_threadPool;
    int m_maxParallel;
    int m_baseMaxParallel;
    bool m_paused;
    bool m_started;
    bool m_memoryUnderPressure;
    double m_memoryPressureThreshold;
};
#endif // TASK_MANAGER_H
