#include "conversion_coordinator.h"

#include "conversion_planner.h"
#include "logger.h"
#include "task_manager.h"

#include <QDir>
#include <QFileInfo>

ConversionCoordinator::ConversionCoordinator(QObject* parent) : QObject(parent)
{ }

int ConversionCoordinator::submit(const QList<CategoryInput>& inputs, const QString& outputDir,
                                  const QSet<QString>& onlyPaths)
{
    QString dir = outputDir;
    if (dir.isEmpty())
    {
        dir = QDir::homePath();
    }
    QDir().mkpath(dir);

    int added = 0;
    for (const CategoryInput& in : inputs)
    {
        if (in.files.isEmpty())
        {
            continue;
        }
        for (const FileInfo& fileInfo : in.files)
        {
            // Retry mode: skip anything not in the requested path set.
            if (!onlyPaths.isEmpty() && !onlyPaths.contains(fileInfo.filePath))
            {
                continue;
            }
            // Output naming + converter routing live in ConversionPlanner
            // (shared with the CLI; analysis §4.2).
            const QString outputFile =
                ConversionPlanner::outputPath(fileInfo.filePath, QString(), dir, in.outputFormat);
            if (QFileInfo(outputFile).fileName().contains(QStringLiteral("_converted.")))
            {
                LOG_WARNING("Coordinator", QStringLiteral("输出路径与输入相同，自动重命名: %1").arg(outputFile));
            }

            QVariantMap params;
            params[QStringLiteral("outputFormat")] = in.outputFormat;
            const QString converter =
                ConversionPlanner::converterNameFor(in.outputFormat, QFileInfo(fileInfo.filePath).suffix());
            params[QStringLiteral("converter")] = converter.isEmpty() ? QStringLiteral("FFmpeg") : converter;

            if (!in.dialogParams.isEmpty())
            {
                params = ConversionPlanner::mergeParams(params, in.dialogParams, in.category);
            }

            TaskManager::instance()->addTask(fileInfo.filePath, outputFile, params);
            ++added;
        }
    }
    if (added > 0)
    {
        LOG_INFO("Coordinator", QStringLiteral("已从所有分类提交 %1 个转换任务").arg(added));
    }
    return added;
}

void ConversionCoordinator::recordCompletedTask(const QString& taskId, bool success)
{
    ConversionTask* task = TaskManager::instance()->getTask(taskId);
    if (!task)
    {
        return;
    }
    m_results.append(
        ConversionResult(task->inputFile(), task->outputFile(), success, task->errorMessage(), task->durationMs()));
    emit resultsChanged();
}

void ConversionCoordinator::clearResults()
{
    m_results.clear();
    emit resultsChanged();
}

void ConversionCoordinator::dropFailedResultsFor(const QSet<QString>& paths)
{
    for (auto it = m_results.begin(); it != m_results.end();)
    {
        if (paths.contains(it->inputPath) && !it->success)
        {
            it = m_results.erase(it);
        }
        else
        {
            ++it;
        }
    }
    emit resultsChanged();
}

QStringList ConversionCoordinator::failedTaskInputs() const
{
    QStringList failed;
    for (ConversionTask* t : TaskManager::instance()->getFailedTasks())
    {
        failed << t->inputFile();
    }
    return failed;
}

bool ConversionCoordinator::togglePause()
{
    TaskManager* tm = TaskManager::instance();
    if (tm->isPaused())
    {
        tm->resume();
    }
    else
    {
        tm->pause();
    }
    return tm->isPaused();
}

bool ConversionCoordinator::isPaused() const
{
    return TaskManager::instance()->isPaused();
}

void ConversionCoordinator::markStarted(const QString& taskId)
{
    ConversionTask* task = TaskManager::instance()->getTask(taskId);
    if (task)
    {
        m_currentFile = QFileInfo(task->inputFile()).fileName();
    }
}

void ConversionCoordinator::markFinished()
{
    m_currentFile.clear();
}

QString ConversionCoordinator::countersText() const
{
    auto c = TaskManager::instance()->counters();
    return QObject::tr("总计 %1 · 运行 %2 · 等待 %3 · 完成 %4 · 失败 %5")
        .arg(c.total)
        .arg(c.running)
        .arg(c.pending)
        .arg(c.completed)
        .arg(c.failed);
}
