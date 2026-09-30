#ifndef CLI_RUNNER_H
#define CLI_RUNNER_H

#include "core/iconverter.h"

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace CliRunner
{

// Parsed command line arguments. Empty / default fields mean "not set".
struct Options
{
    bool showHelp = false;
    bool showVersion = false;
    bool listFormats = false;
    bool verbose = false;
    QString presetFile; // §一.1: share-file recipe to apply (NOT the
                        // x264 --preset encoder knob — name collision
                        // resolved in the strategy audit note)
    QStringList inputs;
    QStringList outputs;          // parallel to inputs; may be empty
    QString outputDir;            // when outputs are not specified explicitly
    QString format;               // target extension (without dot) for --output-dir mode
    QVariantMap conversionParams; // codec / crf / bitrate / preset etc.
    // Rendered by the shared QCommandLineParser (audit A-1): filled by
    // parseArgs even on error so the caller can echo help after a failure.
    QString helpText;
    QString versionText;
};

// Returns Options{} (with showHelp=true) if --help is present, otherwise
// the parsed options. Throws nothing — unknown flags are reported via
// errorMessage and the caller should treat as failure.
Options parseArgs(const QStringList& args, QString* errorMessage = nullptr);

// Run the CLI: pick the right IConverter for each input, dispatch conversions,
// stream progress to stdout. Returns process exit code (0 = success).
// converters is a map of name -> IConverter* (e.g. "FFmpeg", "Pandoc", "ImageMagick").
int run(const Options& opts, const QHash<QString, IConverter*>& convertersByName, QString* errorMessage = nullptr);

// Print the parser-generated help (Options::helpText) to stdout.
void printHelp(const Options& opts);

// Print the app name + version (Options::versionText) to stdout.
void printVersion(const Options& opts);

// Print the supported-format table to stdout.
void printFormats();

} // namespace CliRunner

#endif // CLI_RUNNER_H
