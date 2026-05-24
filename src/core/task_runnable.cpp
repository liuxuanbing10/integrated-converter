#include "task_runnable.h"
#include "logger.h"

TaskRunnable::TaskRunnable(ConversionTask* task, const QString& converterName,
                           std::shared_ptr<IConverter> converter, QObject* parent)
    : QObject(parent)
    , m_task(task)
    , m_converterName(converterName)
    , m_converter(std::move(converter))
    , m_taskId(task ? task->id() : QString())
    , m_running(0)
{
    setAutoDelete(true);
}

TaskRunnable::~TaskRunnable() {
}

void TaskRunnable::run() {
    if (!m_task) {
        emit finished(m_taskId, false, tr("任务无效"));
        return;
    }

    if (m_task->isCancelled()) {
        m_task->setStatus(ConversionTask::Status::Cancelled);
        emit finished(m_taskId, false, tr("任务已取消"));
        return;
    }

    m_running.storeRelaxed(1);
    emit started(m_taskId);

    LOG_INFO("TaskRunnable", QString("开始执行任务: %1, 转换器: %2").arg(m_taskId, m_converterName));

    m_task->setStatus(ConversionTask::Status::Running);
    m_task->setProgress(0);

    bool success = false;
    QString errorMsg;

    if (!m_converter) {
        m_task->setErrorMessage(tr("未找到转换器: %1").arg(m_converterName));
        m_task->setStatus(ConversionTask::Status::Failed);
        emit finished(m_taskId, false, tr("未找到转换器: %1").arg(m_converterName));
        m_running.storeRelaxed(0);
        return;
    }

    try {
        success = m_converter->convert(m_task->inputFile(), m_task->outputFile(), m_task->params());
        if (!success) {
            errorMsg = tr("转换失败");
        }
    } catch (const std::exception& e) {
        success = false;
        errorMsg = QString::fromLocal8Bit(e.what());
        LOG_ERROR("TaskRunnable", QString("任务异常: %1 - %2").arg(m_taskId).arg(errorMsg));
    } catch (...) {
        success = false;
        errorMsg = tr("未知异常");
        LOG_ERROR("TaskRunnable", QString("任务未知异常: %1").arg(m_taskId));
    }

    m_running.storeRelaxed(0);

    if (m_task->isCancelled()) {
        m_task->setStatus(ConversionTask::Status::Cancelled);
        emit finished(m_taskId, false, tr("任务已取消"));
        LOG_INFO("TaskRunnable", QString("任务已取消: %1").arg(m_taskId));
    } else if (success) {
        m_task->setStatus(ConversionTask::Status::Completed);
        m_task->setProgress(100);
        emit finished(m_taskId, true, QString());
        LOG_INFO("TaskRunnable", QString("任务完成: %1").arg(m_taskId));
    } else {
        m_task->setErrorMessage(errorMsg);
        m_task->setStatus(ConversionTask::Status::Failed);
        emit finished(m_taskId, false, errorMsg);
        LOG_ERROR("TaskRunnable", QString("任务失败: %1 - %2").arg(m_taskId).arg(errorMsg));
    }
}
