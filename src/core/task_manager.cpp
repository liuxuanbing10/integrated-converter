#include "task_manager.h"
#include "task_runnable.h"
#include "config_manager.h"
#include "logger.h"
#include "memory_monitor.h"
#include "large_file_handler.h"
#include <QFileInfo>
#include <QCoreApplication>

TaskManager* TaskManager::instance() {
    static TaskManager s_instance;
    return &s_instance;
}

TaskManager::TaskManager()
    : m_threadPool(new QThreadPool(this))
    , m_maxParallel(4)
    , m_baseMaxParallel(4)
    , m_paused(false)
    , m_started(false)
    , m_memoryUnderPressure(false)
    , m_memoryPressureThreshold(0.75)
{
    m_maxParallel = ConfigManager::instance().maxParallelTasks();
    m_baseMaxParallel = m_maxParallel;
    m_threadPool->setMaxThreadCount(m_maxParallel);
    LOG_INFO("TaskManager", QString("任务管理器初始化，最大并行数: %1").arg(m_maxParallel));
    MemoryMonitor* memMonitor = MemoryMonitor::instance();
    connect(memMonitor, &MemoryMonitor::memoryWarning,
            this, &TaskManager::onMemoryWarning, Qt::QueuedConnection);
    connect(memMonitor, &MemoryMonitor::memoryCritical,
            this, &TaskManager::onMemoryCritical, Qt::QueuedConnection);
    connect(memMonitor, &MemoryMonitor::memoryNormalized,
            this, &TaskManager::onMemoryNormalized, Qt::QueuedConnection);
    memMonitor->startMonitoring();
}

TaskManager::~TaskManager() {
    QMutexLocker locker(&m_mutex);
    cancelAllTasks();
    m_threadPool->waitForDone(5000);
    qDeleteAll(m_tasks);
    m_tasks.clear();
    m_runningTasks.clear();
    m_pendingQueue.clear();
}

void TaskManager::registerConverter(const QString& name, std::shared_ptr<IConverter> converter) {
    QMutexLocker locker(&m_mutex);
    m_converters[name] = converter;
    LOG_INFO("TaskManager", QString("注册转换器: %1").arg(name));
}

void TaskManager::unregisterConverter(const QString& name) {
    QMutexLocker locker(&m_mutex);
    m_converters.remove(name);
    LOG_INFO("TaskManager", QString("注销转换器: %1").arg(name));
}

QStringList TaskManager::availableConverters() const {
    QMutexLocker locker(&m_mutex);
    return m_converters.keys();
}

IConverter* TaskManager::converter(const QString& name) const {
    QMutexLocker locker(&m_mutex);
    return m_converters.value(name).get();
}

void TaskManager::insertTaskByPriority(const QString& taskId) {
    ConversionTask* task = m_tasks.value(taskId);
    if (!task) {
        m_pendingQueue.append(taskId);
        return;
    }
    int priority = static_cast<int>(task->priority());
    int insertPos = m_pendingQueue.size();
    for (int i = 0; i < m_pendingQueue.size(); ++i) {
        ConversionTask* existingTask = m_tasks.value(m_pendingQueue[i]);
        if (existingTask && static_cast<int>(existingTask->priority()) < priority) {
            insertPos = i;
            break;
        }
    }
    m_pendingQueue.insert(insertPos, taskId);
}

void TaskManager::updateTaskPriority(const QString& taskId) {
    ConversionTask* task = m_tasks.value(taskId);
    if (!task) return;
    qint64 fileSize = LargeFileHandler::getFileSize(task->inputFile());
    task->setFileSize(fileSize);
    auto category = LargeFileHandler::categorizeBySize(fileSize);
    int recommendedPriority = LargeFileHandler::recommendedPriority(category);
    if (recommendedPriority < static_cast<int>(task->priority())) {
        task->setPriority(static_cast<ConversionTask::Priority>(recommendedPriority));
        LOG_DEBUG("TaskManager", QString("根据文件大小调整任务优先级: %1 -> %2")
                 .arg(taskId)
                 .arg(ConversionTask::priorityToString(task->priority())));
    }
}

int TaskManager::calculateDynamicMaxParallel() const {
    if (m_memoryUnderPressure) {
        return qMax(1, m_baseMaxParallel / 2);
    }
    MemoryMonitor* memMonitor = MemoryMonitor::instance();
    double usageRatio = memMonitor->usageRatio();
    if (usageRatio > m_memoryPressureThreshold) {
        int reduced = static_cast<int>(m_baseMaxParallel * (1.0 - (usageRatio - m_memoryPressureThreshold)));
        return qMax(1, reduced);
    }
    return m_baseMaxParallel;
}

QString TaskManager::addTask(std::unique_ptr<ConversionTask> task) {
    if (!task) {
        return QString();
    }
    QString taskId;
    bool shouldProcess = false;
    {
        QMutexLocker locker(&m_mutex);
        taskId = task->id();
        // Take ownership: release from unique_ptr, store as raw pointer internally.
        // QMap does not support move-only types (COW semantics), so we manage
        // lifetime manually via qDeleteAll in the destructor / deleteLater in removeTask.
        m_tasks[taskId] = task.release();
        updateTaskPriority(taskId);
        insertTaskByPriority(taskId);
        shouldProcess = m_started && !m_paused;
        LOG_INFO("TaskManager", QString("添加任务: %1, 优先级: %2")
                 .arg(taskId)
                 .arg(ConversionTask::priorityToString(m_tasks.value(taskId)->priority())));
    }
    // Emit signal OUTSIDE mutex to prevent deadlock when connected slot calls back into TaskManager
    emit taskAdded(taskId);
    if (shouldProcess) {
        processQueue();
    }
    return taskId;
}

QString TaskManager::addTask(const QString& inputFile, const QString& outputFile,
                             const QVariantMap& params) {
    auto task = std::make_unique<ConversionTask>(inputFile, outputFile, params);
    QString converterName = params.value("converter").toString();
    task->setConverterType(ConversionTask::stringToConverterType(converterName));
    if (params.contains("priority")) {
        int priorityVal = params.value("priority").toInt();
        task->setPriority(static_cast<ConversionTask::Priority>(
            qBound(0, priorityVal, static_cast<int>(ConversionTask::Priority::High))));
    }
    return addTask(std::move(task));
}

void TaskManager::removeTask(const QString& taskId) {
    {
        QMutexLocker locker(&m_mutex);
        if (!m_tasks.contains(taskId)) {
            return;
        }
        // Cancel inline to avoid re-entering mutex via cancelTask()
        ConversionTask* task = m_tasks.value(taskId);
        if (task) {
            ConversionTask::Status status = task->status();
            if (status == ConversionTask::Status::Running) {
                task->requestCancel();
            } else if (status == ConversionTask::Status::Pending) {
                task->setStatus(ConversionTask::Status::Cancelled);
                m_pendingQueue.removeAll(taskId);
            }
        }
        task = m_tasks.take(taskId);
        m_runningTasks.remove(taskId);
        if (task) {
            task->deleteLater();
        }
        LOG_INFO("TaskManager", QString("移除任务: %1").arg(taskId));
    }
    emit taskRemoved(taskId);
}

void TaskManager::cancelTask(const QString& taskId) {
    QMutexLocker locker(&m_mutex);
    ConversionTask* task = m_tasks.value(taskId);
    if (!task) {
        return;
    }
    ConversionTask::Status status = task->status();
    if (status == ConversionTask::Status::Running) {
        task->requestCancel();
        LOG_INFO("TaskManager", QString("请求取消运行中任务: %1").arg(taskId));
    } else if (status == ConversionTask::Status::Pending) {
        task->setStatus(ConversionTask::Status::Cancelled);
        m_pendingQueue.removeAll(taskId);
        LOG_INFO("TaskManager", QString("取消等待中任务: %1").arg(taskId));
    }
}

void TaskManager::cancelAllTasks() {
    QMutexLocker locker(&m_mutex);
    for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
        ConversionTask* task = it.value();
        if (task) {
            ConversionTask::Status status = task->status();
            if (status == ConversionTask::Status::Running) {
                task->requestCancel();
            } else if (status == ConversionTask::Status::Pending) {
                task->setStatus(ConversionTask::Status::Cancelled);
            }
        }
    }
    m_pendingQueue.clear();
    m_threadPool->clear();
    LOG_INFO("TaskManager", "取消所有任务");
}

ConversionTask* TaskManager::getTask(const QString& taskId) const {
    QMutexLocker locker(&m_mutex);
    return m_tasks.value(taskId);
}

QList<ConversionTask*> TaskManager::getAllTasks() const {
    QMutexLocker locker(&m_mutex);
    return m_tasks.values();
}

QList<ConversionTask*> TaskManager::getPendingTasks() const {
    QMutexLocker locker(&m_mutex);
    QList<ConversionTask*> result;
    for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
        if (it.value() && it.value()->status() == ConversionTask::Status::Pending) {
            result.append(it.value());
        }
    }
    return result;
}

QList<ConversionTask*> TaskManager::getRunningTasks() const {
    QMutexLocker locker(&m_mutex);
    QList<ConversionTask*> result;
    for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
        if (it.value() && it.value()->status() == ConversionTask::Status::Running) {
            result.append(it.value());
        }
    }
    return result;
}

QList<ConversionTask*> TaskManager::getCompletedTasks() const {
    QMutexLocker locker(&m_mutex);
    QList<ConversionTask*> result;
    for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
        if (it.value() && it.value()->status() == ConversionTask::Status::Completed) {
            result.append(it.value());
        }
    }
    return result;
}

QList<ConversionTask*> TaskManager::getFailedTasks() const {
    QMutexLocker locker(&m_mutex);
    QList<ConversionTask*> result;
    for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
        ConversionTask* task = it.value();
        if (task) {
            ConversionTask::Status status = task->status();
            if (status == ConversionTask::Status::Failed || status == ConversionTask::Status::Cancelled) {
                result.append(task);
            }
        }
    }
    return result;
}

void TaskManager::start() {
    {
        QMutexLocker locker(&m_mutex);
        m_started = true;
        m_paused = false;
        LOG_INFO("TaskManager", "启动任务管理器");
    }
    processQueue();
}

void TaskManager::pause() {
    QMutexLocker locker(&m_mutex);
    m_paused = true;
    LOG_INFO("TaskManager", "暂停任务管理器");
}

void TaskManager::resume() {
    bool shouldProcess = false;
    {
        QMutexLocker locker(&m_mutex);
        if (m_paused) {
            m_paused = false;
            shouldProcess = true;
            LOG_INFO("TaskManager", "恢复任务管理器");
        }
    }
    if (shouldProcess) {
        processQueue();
    }
}

bool TaskManager::isPaused() const {
    QMutexLocker locker(&m_mutex);
    return m_paused;
}

bool TaskManager::isRunning() const {
    QMutexLocker locker(&m_mutex);
    return m_started && !m_paused;
}

void TaskManager::setMaxParallelTasks(int max) {
    QMutexLocker locker(&m_mutex);
    m_maxParallel = qBound(1, max, QThread::idealThreadCount() * 2);
    m_baseMaxParallel = m_maxParallel;
    m_threadPool->setMaxThreadCount(m_maxParallel);
    LOG_INFO("TaskManager", QString("设置最大并行数: %1").arg(m_maxParallel));
}

int TaskManager::maxParallelTasks() const {
    QMutexLocker locker(&m_mutex);
    return m_maxParallel;
}

int TaskManager::totalTaskCount() const {
    QMutexLocker locker(&m_mutex);
    return m_tasks.size();
}

int TaskManager::pendingCount() const {
    QMutexLocker locker(&m_mutex);
    int count = 0;
    for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
        if (it.value() && it.value()->status() == ConversionTask::Status::Pending) {
            ++count;
        }
    }
    return count;
}

int TaskManager::runningCount() const {
    QMutexLocker locker(&m_mutex);
    int count = 0;
    for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
        if (it.value() && it.value()->status() == ConversionTask::Status::Running) {
            ++count;
        }
    }
    return count;
}

int TaskManager::completedCount() const {
    QMutexLocker locker(&m_mutex);
    int count = 0;
    for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
        if (it.value() && it.value()->status() == ConversionTask::Status::Completed) {
            ++count;
        }
    }
    return count;
}

int TaskManager::failedCount() const {
    QMutexLocker locker(&m_mutex);
    int count = 0;
    for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
        ConversionTask* task = it.value();
        if (task) {
            ConversionTask::Status status = task->status();
            if (status == ConversionTask::Status::Failed || status == ConversionTask::Status::Cancelled) {
                ++count;
            }
        }
    }
    return count;
}

bool TaskManager::isMemoryUnderPressure() const {
    return m_memoryUnderPressure;
}

void TaskManager::setMemoryPressureThreshold(double ratio) {
    m_memoryPressureThreshold = qBound(0.5, ratio, 0.95);
}

void TaskManager::adjustParallelTasksForMemory() {
    int newMax = calculateDynamicMaxParallel();
    if (newMax != m_maxParallel) {
        m_maxParallel = newMax;
        m_threadPool->setMaxThreadCount(m_maxParallel);
        LOG_INFO("TaskManager", QString("根据内存压力调整并行数: %1").arg(m_maxParallel));
    }
}

void TaskManager::processQueue() {
    QMutexLocker locker(&m_mutex);
    if (m_paused || !m_started) {
        return;
    }
    int effectiveMaxParallel = calculateDynamicMaxParallel();
    while (m_runningTasks.size() < effectiveMaxParallel && !m_pendingQueue.isEmpty()) {
        QString taskId = m_pendingQueue.takeFirst();
        ConversionTask* task = m_tasks.value(taskId);
        if (!task || task->status() != ConversionTask::Status::Pending) {
            continue;
        }
        if (task->isCancelled()) {
            task->setStatus(ConversionTask::Status::Cancelled);
            continue;
        }
        QString converterName = task->params().value("converter").toString();
        if (!m_converters.contains(converterName)) {
            task->setErrorMessage(tr("未找到转换器: %1").arg(converterName));
            task->setStatus(ConversionTask::Status::Failed);
            LOG_ERROR("TaskManager", QString("未找到转换器: %1").arg(converterName));
            continue;
        }
        auto converterPtr = m_converters.value(converterName);
        TaskRunnable* runnable = new TaskRunnable(task, converterName, converterPtr);
        m_runningTasks[taskId] = runnable;
        connect(runnable, &TaskRunnable::started,
                this, &TaskManager::onTaskStarted, Qt::QueuedConnection);
        connect(runnable, &TaskRunnable::progressChanged,
                this, &TaskManager::onTaskProgressChanged, Qt::QueuedConnection);
        connect(runnable, &TaskRunnable::finished,
                this, &TaskManager::onTaskFinished, Qt::QueuedConnection);
        m_threadPool->start(runnable, static_cast<int>(task->priority()));
        LOG_INFO("TaskManager", QString("提交任务到线程池: %1").arg(taskId));
    }
}

void TaskManager::onTaskStarted(const QString& taskId) {
    LOG_INFO("TaskManager", QString("任务开始: %1").arg(taskId));
    emit taskStarted(taskId);
}

void TaskManager::onTaskProgressChanged(const QString& taskId, int progress) {
    emit taskProgressChanged(taskId, progress);
}

void TaskManager::onTaskFinished(const QString& taskId, bool success, const QString& message) {
    Q_UNUSED(message);
    bool allDone = false;
    {
        QMutexLocker locker(&m_mutex);
        m_runningTasks.remove(taskId);
        LOG_INFO("TaskManager", QString("任务完成: %1, 成功: %2").arg(taskId).arg(success));
        bool hasPending = false;
        for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
            if (it.value() && it.value()->status() == ConversionTask::Status::Pending) {
                hasPending = true;
                break;
            }
        }
        allDone = m_runningTasks.isEmpty() && !hasPending;
    }
    // Emit signals OUTSIDE mutex
    emit taskCompleted(taskId, success);
    if (allDone) {
        LOG_INFO("TaskManager", "所有任务已完成");
        emit allTasksCompleted();
    } else {
        processQueue();
    }
}

void TaskManager::onMemoryWarning(size_t current, size_t threshold) {
    Q_UNUSED(current);
    Q_UNUSED(threshold);
    LOG_WARNING("TaskManager", "内存警告，准备减少并行任务数");
    m_memoryUnderPressure = true;
    adjustParallelTasksForMemory();
    emit memoryPressureChanged(true);
}

void TaskManager::onMemoryCritical(size_t current, size_t threshold) {
    Q_UNUSED(current);
    Q_UNUSED(threshold);
    LOG_ERROR("TaskManager", "内存严重警告，暂停新任务启动");
    m_memoryUnderPressure = true;
    m_maxParallel = 1;
    m_threadPool->setMaxThreadCount(1);
    emit memoryPressureChanged(true);
}

void TaskManager::onMemoryNormalized() {
    {
        QMutexLocker locker(&m_mutex);
        LOG_INFO("TaskManager", "内存恢复正常，恢复并行任务数");
        m_memoryUnderPressure = false;
        m_maxParallel = m_baseMaxParallel;
        m_threadPool->setMaxThreadCount(m_maxParallel);
    }
    emit memoryPressureChanged(false);
    processQueue();
}
