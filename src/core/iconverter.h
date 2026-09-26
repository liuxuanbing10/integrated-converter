#ifndef ICONVERTER_H
#define ICONVERTER_H
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QAtomicInt>
#include <memory>
#include <optional>
#include <functional>
#include "error_types.h"

// Optional per-task progress callback (0-100). Injected by TaskRunnable before
// convert(); invoked from the worker thread, so implementations must only
// touch per-instance state.
using ProgressFn = std::function<void(int)>;

class IConverter {
public:
    virtual ~IConverter() = default;

    // Converters carry per-run state (QProcess, progress buffers, last error)
    // that is NOT safe to share across concurrent tasks. TaskManager clones
    // one independent instance per queued task instead of dispatching N tasks
    // onto one shared singleton.
    virtual std::unique_ptr<IConverter> clone() const = 0;

    // Cooperative cancellation: the owning ConversionTask's atomic cancel flag
    // is injected before convert(); the running wait-loop polls it and kills
    // its OWN child process on the worker thread. Cancellation is per-task
    // routed — cancelling A can never kill B's process, and the caller
    // (GUI thread) only flips an atomic instead of holding a mutex over a
    // blocking kill()/waitForFinished().
    virtual void setCancelFlag(const QAtomicInt* flag) { m_cancelFlag = flag; }

    // Real-time progress reporting (percentage 0-100). Default no-op for
    // converters with no granular progress (Pandoc, coarse IM).
    virtual void setProgressCallback(ProgressFn cb) { m_progressCb = std::move(cb); }

    virtual std::optional<ErrorInfo> convert(const QString& inputFile, const QString& outputFile,
                                              const QVariantMap& params) = 0;
    virtual QStringList supportedInputFormats() const = 0;
    virtual QStringList supportedOutputFormats() const = 0;
    virtual QString name() const = 0;
    virtual bool isConversionSupported(const QString& inputFormat,
                                       const QString& outputFormat) const = 0;
    // Synchronous hard-cancel for direct (non-TaskManager) users, e.g. CLI and
    // destructors. TaskManager paths use setCancelFlag() instead.
    virtual void cancel() {}

protected:
    void reportProgress(int percent) {
        if (m_progressCb) m_progressCb(qBound(0, percent, 100));
    }
    bool isCancelRequested() const {
        return m_cancelFlag && m_cancelFlag->loadAcquire() != 0;
    }

    const QAtomicInt* m_cancelFlag = nullptr;
    ProgressFn m_progressCb;
};
#endif // ICONVERTER_H
