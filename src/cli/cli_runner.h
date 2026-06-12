#ifndef CLI_RUNNER_H
#define CLI_RUNNER_H

#include <QString>
#include <QStringList>
#include <QVariantMap>
#include "core/iconverter.h"

namespace CliRunner {

// Parsed command line arguments. Empty / default fields mean "not set".
struct Options {
    bool showHelp = false;
    bool listFormats = false;
    bool verbose = false;
    QStringList inputs;
    QStringList outputs;       // parallel to inputs; may be empty
    QString outputDir;         // when outputs are not specified explicitly
    QString format;            // target extension (without dot) for --output-dir mode
    QVariantMap conversionParams;  // codec / crf / bitrate / preset etc.
};

// Returns Options{} (with showHelp=true) if --help is present, otherwise
// the parsed options. Throws nothing — unknown flags are reported via
// errorMessage and the caller should treat as failure.
Options parseArgs(const QStringList& args, QString* errorMessage = nullptr);

// Run the CLI: pick the right IConverter for each input, dispatch conversions,
// stream progress to stdout. Returns process exit code (0 = success).
// converters is a map of name -> IConverter* (e.g. "FFmpeg", "Pandoc", "ImageMagick").
int run(const Options& opts,
        const QHash<QString, IConverter*>& convertersByName,
        QString* errorMessage = nullptr);

// Print --help text to stdout.
void printHelp();

// Print the supported-format table to stdout.
void printFormats();

} // namespace CliRunner

#endif // CLI_RUNNER_H
