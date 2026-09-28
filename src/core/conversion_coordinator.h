#ifndef CONVERSION_COORDINATOR_H
#define CONVERSION_COORDINATOR_H

#include "conversion_result.h"
#include "file_info.h"
#include "format_registry.h"

#include <QList>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QString>
#include <QVariantMap>

/// §零.2 (analysis §10.11): the submit/route/stats/pause orchestration that
/// used to live inside MainWindow. Zero widget dependencies — MainWindow
/// gathers plain data, forwards TaskManager signals, and renders coordinator
/// output. All conversion semantics (output naming, converter routing, param
/// normalization) still live in ConversionPlanner; this class is the session
/// ledger + dispatch shell around it.
class ConversionCoordinator : public QObject
{
    Q_OBJECT
public:
    struct CategoryInput
    {
        FormatRegistry::Category category = FormatRegistry::Category::Image;
        QList<FileInfo> files;
        QString outputFormat;     // resolved per-category by the caller
        QVariantMap dialogParams; // saved params from the params dialog
    };

    explicit ConversionCoordinator(QObject* parent = nullptr);

    /// Expand category inputs into TaskManager tasks. When onlyPaths is
    /// non-empty, submit ONLY those input paths (retry-failed flow).
    /// Returns the number of tasks added.
    int submit(const QList<CategoryInput>& inputs, const QString& outputDir, const QSet<QString>& onlyPaths = {});

    /// Append the ledger row for a finished task (taskCompleted signal).
    void recordCompletedTask(const QString& taskId, bool success);

    const QList<ConversionResult>& results() const
    {
        return m_results;
    }
    bool hasResults() const
    {
        return !m_results.isEmpty();
    }
    void clearResults();

    /// Retry housekeeping: drop stale FAILURE rows for the given input paths
    /// so the fresh attempt shows once instead of duplicating. Successes stay.
    void dropFailedResultsFor(const QSet<QString>& paths);

    /// Input paths of every failed task currently in TaskManager.
    QStringList failedTaskInputs() const;

    /// Queue-pause semantics (3FUI): pause = stop dispatching NEW tasks;
    /// already-running conversions are untouched. Returns the new state.
    bool togglePause();
    bool isPaused() const;

    /// Last task that started — the progress widget header line.
    void markStarted(const QString& taskId);
    void markFinished();
    QString currentFile() const
    {
        return m_currentFile;
    }

    /// Translated queue counters for the status bar.
    QString countersText() const;

signals:
    void resultsChanged();

private:
    QList<ConversionResult> m_results;
    QString m_currentFile;
};

#endif // CONVERSION_COORDINATOR_H
