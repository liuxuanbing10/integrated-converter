#ifndef CONVERSION_RESULT_H
#define CONVERSION_RESULT_H

#include <QString>

/// One finished conversion attempt — the ledger row the batch summary shows
/// and the retry flow prunes. Moved out of ui/batch_conversion_summary.h so
/// ConversionCoordinator (core/) can own the ledger without a UI dependency
/// (§零.2 / analysis §10.11).
struct ConversionResult
{
    QString inputPath;
    QString outputPath;
    bool success;
    QString errorMessage;
    qint64 durationMs;
    ConversionResult() : success(false), durationMs(0)
    { }
    ConversionResult(const QString& in, const QString& out, bool ok, const QString& err, qint64 dur) :
        inputPath(in), outputPath(out), success(ok), errorMessage(err), durationMs(dur)
    { }
};

#endif // CONVERSION_RESULT_H
