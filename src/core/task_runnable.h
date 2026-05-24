#ifndef TASK_RUNNABLE_H
#define TASK_RUNNABLE_H

#include <QRunnable>
#include <QObject>
#include <QAtomicInt>
#include "conversion_task.h"
#include "iconverter.h"

class TaskRunnable : public QObject, public QRunnable {
    Q_OBJECT

public:
    explicit TaskRunnable(ConversionTask* task, const QString& converterName,
                          std::shared_ptr<IConverter> converter, QObject* parent = nullptr);
    ~TaskRunnable() override;

    void run() override;
    QString taskId() const { return m_taskId; }
    bool isRunning() const { return m_running.loadRelaxed() != 0; }

signals:
    void started(const QString& taskId);
    void progressChanged(const QString& taskId, int progress);
    void finished(const QString& taskId, bool success, const QString& message);

private:
    ConversionTask* m_task;
    QString m_converterName;
    std::shared_ptr<IConverter> m_converter;
    QString m_taskId;
    QAtomicInt m_running;
};

#endif // TASK_RUNNABLE_H
