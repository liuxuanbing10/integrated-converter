#include "cli_runner.h"

#include "core/conversion_planner.h"
#include "core/format_registry.h"
#include "core/iconverter.h"
#include "core/logger.h"
#include "core/preset.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QStringList>
#include <QTextStream>

#include <cstdio>

namespace CliRunner
{

namespace
{

void printOut(const QString& s)
{
    const QByteArray utf8 = s.toUtf8();
    fwrite(utf8.constData(), 1, utf8.size(), stdout);
    fflush(stdout);
}

void printErr(const QString& s)
{
    const QByteArray utf8 = s.toUtf8();
    fwrite(utf8.constData(), 1, utf8.size(), stderr);
    fflush(stderr);
}

void printOutLine(const QString& s)
{
    printOut(s + "\n");
}

} // namespace

Options parseArgs(const QStringList& args, QString* errorMessage)
{
    Options opts;
    auto fail = [&](const QString& msg) {
        if (errorMessage)
            *errorMessage = msg;
        return opts;
    };

    // Audit A-1: QCommandLineParser replaces the 126-line hand-rolled loop.
    // parse() (not process()) — the hand-rolled contract never exits() from
    // inside parsing; main owns exit codes (0 ok / 1 run-failure / 2 usage).
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Integrated Format Converter - CLI mode"));
    parser.addHelpOption();
    // Qt 6.12 regression trap: addVersionOption() now OWNS the -v short form,
    // which collides with our advertised "--verbose/-v" (the verbose option
    // silently fails to register — CI caught it via "option already added: v").
    // Register version long-name-only so -v stays verbose (baseline contract).
    parser.addOption(QCommandLineOption(QStringLiteral("version"), QStringLiteral("Show version information.")));
    parser.addOptions({
        {{QStringLiteral("i"), QStringLiteral("input")},
         QStringLiteral("Input file (repeatable, or pass positionally)"),
         QStringLiteral("path")},
        {{QStringLiteral("o"), QStringLiteral("output")},
         QStringLiteral("Output file (repeatable, must match --input count)"),
         QStringLiteral("path")},
        {QStringLiteral("output-dir"), QStringLiteral("Directory to write outputs to (use with --format)"),
         QStringLiteral("dir")},
        {{QStringLiteral("f"), QStringLiteral("format")},
         QStringLiteral("Target format/extension (e.g. mp3, mp4, png)"),
         QStringLiteral("ext")},
        {QStringLiteral("codec"), QStringLiteral("Video codec (e.g. libx264, libx265, libvpx-vp9)"),
         QStringLiteral("name")},
        {QStringLiteral("audio-codec"), QStringLiteral("Audio codec (e.g. aac, libmp3lame, libopus)"),
         QStringLiteral("name")},
        {QStringLiteral("crf"), QStringLiteral("Constant rate factor (0-51, lower = better)"), QStringLiteral("int")},
        {QStringLiteral("bitrate"), QStringLiteral("Video bitrate"), QStringLiteral("kbps")},
        {QStringLiteral("audio-bitrate"), QStringLiteral("Audio bitrate"), QStringLiteral("kbps")},
        {QStringLiteral("preset"), QStringLiteral("Encoder preset (ultrafast..veryslow)"), QStringLiteral("name")},
        // §一.1: apply a preset share file. Strategy text said `--preset
        // file.json`, but that name is taken by the x264 encoder knob above —
        // reusing it would silently retype an existing flag. `--preset-file`
        // it is; README documents both so no user guesses wrong.
        {QStringLiteral("preset-file"),
         QStringLiteral("Apply a preset .json share file (its format may fill --format)"), QStringLiteral("file")},
        {{QStringLiteral("s"), QStringLiteral("resolution")},
         QStringLiteral("Output resolution (e.g. 1920x1080)"),
         QStringLiteral("WxH")},
        {QStringLiteral("list-formats"), QStringLiteral("Print all supported formats and exit")},
        {{QStringLiteral("v"), QStringLiteral("verbose")}, QStringLiteral("Verbose logging to stderr")},
    });

    // parse() expects argv[0] to be the executable name.
    QStringList fullArgs;
    fullArgs << QStringLiteral("integrated_converter") << args;
    const bool parsed = parser.parse(fullArgs);
    opts.helpText = parser.helpText();
    opts.versionText =
        QStringLiteral("%1 %2").arg(QCoreApplication::applicationName(), QCoreApplication::applicationVersion());
    if (!parsed)
    {
        return fail(parser.errorText());
    }
    opts.showHelp = parser.isSet(QStringLiteral("help"));
    opts.showVersion = parser.isSet(QStringLiteral("version"));
    opts.listFormats = parser.isSet(QStringLiteral("list-formats"));
    opts.verbose = parser.isSet(QStringLiteral("verbose"));
    if (opts.showHelp || opts.showVersion || opts.listFormats)
    {
        return opts; // info flags short-circuit validation (baseline contract)
    }

    // QCommandLineParser separates option values from positionals, so the old
    // traversal-interleave order of "-i a b -i c" cannot be reconstructed.
    // Pairing ambiguity is worse than a usage error (玉衡: 宁可报错，不可错配):
    // fail loudly when explicit --input values and positionals are mixed.
    // Positionals ALONE remain fully supported (README: --cli *.png ...).
    opts.inputs = parser.values(QStringLiteral("input"));
    const QStringList positional = parser.positionalArguments();
    opts.outputs = parser.values(QStringLiteral("output"));
    if (!positional.isEmpty())
    {
        if (!opts.inputs.isEmpty())
        {
            return fail(QStringLiteral("Cannot mix --input with positional files (pairing order is undefined). "
                                       "Use repeated --input/--output, or positionals with --output-dir + --format."));
        }
        opts.inputs = positional;
    }
    opts.outputDir = parser.value(QStringLiteral("output-dir"));
    opts.format = parser.value(QStringLiteral("format")).toLower();
    if (opts.format.startsWith(QStringLiteral(".")))
        opts.format = opts.format.mid(1);

    // §一.1 CLI preset application: explicit flags always win over the file,
    // the file fills gaps (--format omitted -> preset.format; params merged
    // under the same rule ConversionPlanner::mergeParams uses for dialogs).
    opts.presetFile = parser.value(QStringLiteral("preset-file"));
    if (!opts.presetFile.isEmpty())
    {
        QFile pf(opts.presetFile);
        if (!pf.open(QIODevice::ReadOnly))
        {
            return fail(QStringLiteral("--preset-file: cannot read %1").arg(opts.presetFile));
        }
        QJsonParseError pe{};
        const QJsonDocument doc = QJsonDocument::fromJson(pf.readAll(), &pe);
        Preset preset;
        QString reason;
        if (pe.error != QJsonParseError::NoError || !doc.isObject() ||
            !Preset::fromShareJson(doc.object(), &preset, &reason))
        {
            return fail(QStringLiteral("--preset-file: %1 (%2)").arg(opts.presetFile, reason));
        }
        if (opts.format.isEmpty())
        {
            opts.format = preset.format;
        }
        QVariantMap merged = preset.params;
        // CLI flags override preset params: re-collect the set flags on top.
        auto over = [&](const QString& key, const QString& name) {
            if (parser.isSet(name))
                merged[key] = parser.value(name);
        };
        over(QStringLiteral("videoCodec"), QStringLiteral("codec"));
        over(QStringLiteral("audioCodec"), QStringLiteral("audio-codec"));
        over(QStringLiteral("preset"), QStringLiteral("preset"));
        over(QStringLiteral("resolution"), QStringLiteral("resolution"));
        if (parser.isSet(QStringLiteral("crf")))
            merged[QStringLiteral("crf")] = parser.value(QStringLiteral("crf")).toInt();
        if (parser.isSet(QStringLiteral("bitrate")))
            merged[QStringLiteral("videoBitrate")] = parser.value(QStringLiteral("bitrate")).toInt();
        if (parser.isSet(QStringLiteral("audio-bitrate")))
            merged[QStringLiteral("audioBitrate")] = parser.value(QStringLiteral("audio-bitrate")).toInt();
        opts.conversionParams = merged;
    }

    auto put = [&](const QString& key, const QString& name) {
        if (parser.isSet(name))
            opts.conversionParams[key] = parser.value(name);
    };
    put(QStringLiteral("videoCodec"), QStringLiteral("codec"));
    put(QStringLiteral("audioCodec"), QStringLiteral("audio-codec"));
    put(QStringLiteral("preset"), QStringLiteral("preset"));
    put(QStringLiteral("resolution"), QStringLiteral("resolution"));
    if (parser.isSet(QStringLiteral("crf")))
        opts.conversionParams[QStringLiteral("crf")] = parser.value(QStringLiteral("crf")).toInt();
    if (parser.isSet(QStringLiteral("bitrate")))
        opts.conversionParams[QStringLiteral("videoBitrate")] = parser.value(QStringLiteral("bitrate")).toInt();
    if (parser.isSet(QStringLiteral("audio-bitrate")))
        opts.conversionParams[QStringLiteral("audioBitrate")] = parser.value(QStringLiteral("audio-bitrate")).toInt();

    if (opts.inputs.isEmpty())
    {
        return fail(QStringLiteral("No input files. Use --input <path> or pass files positionally."));
    }
    if (opts.outputs.isEmpty() && opts.outputDir.isEmpty())
    {
        return fail(QStringLiteral("Either --output (one per input) or --output-dir + --format is required."));
    }
    if (!opts.outputs.isEmpty() && opts.outputs.size() != opts.inputs.size())
    {
        return fail(QStringLiteral("--output count (%1) must match --input count (%2).")
                        .arg(opts.outputs.size())
                        .arg(opts.inputs.size()));
    }
    if (!opts.outputDir.isEmpty() && opts.format.isEmpty())
    {
        return fail(QStringLiteral("--output-dir requires --format <ext>."));
    }
    return opts;
}

int run(const Options& opts, const QHash<QString, IConverter*>& convertersByName, QString* errorMessage)
{
    if (opts.inputs.isEmpty())
    {
        if (errorMessage)
            *errorMessage = "No input files.";
        return 1;
    }
    const auto& reg = FormatRegistry::instance();
    int failures = 0;
    int total = opts.inputs.size();

    for (int idx = 0; idx < total; ++idx)
    {
        const QString& input = opts.inputs[idx];
        QString output = ConversionPlanner::outputPath(input, idx < opts.outputs.size() ? opts.outputs[idx] : QString(),
                                                       opts.outputDir, opts.format);
        QFileInfo inInfo(input);
        if (!inInfo.exists())
        {
            printErr(QStringLiteral("[skip] %1: file does not exist\n").arg(input));
            ++failures;
            continue;
        }
        const QString converterName =
            ConversionPlanner::converterNameFor(QFileInfo(output).suffix().toLower(), inInfo.suffix().toLower());
        if (converterName.isEmpty())
        {
            printErr(QStringLiteral("[skip] %1: no converter for format\n").arg(input));
            ++failures;
            continue;
        }
        auto it = convertersByName.find(converterName);
        if (it == convertersByName.end())
        {
            printErr(QStringLiteral("[skip] %1: converter %2 not registered\n").arg(input, converterName));
            ++failures;
            continue;
        }
        auto* converter = it.value();

        printOut(QStringLiteral("[%1/%2] %3 -> %4 (%5)").arg(idx + 1).arg(total).arg(input, output, converterName));

        // Ensure output directory exists.
        QFileInfo outInfo(output);
        if (!outInfo.absoluteDir().exists())
        {
            QDir().mkpath(outInfo.absolutePath());
        }

        auto result = converter->convert(input, output, opts.conversionParams);
        if (!result.has_value())
        {
            printOutLine(QStringLiteral("  [ok]"));
            LOG_INFO("CLI", QString("OK: %1 -> %2").arg(input, output));
        }
        else
        {
            QString msg = result->fullMessage();
            printOutLine(QStringLiteral("  [FAILED] %1").arg(msg));
            LOG_ERROR("CLI", QString("FAIL: %1 -> %2 - %3").arg(input, output, msg));
            ++failures;
        }
    }
    printOutLine(QStringLiteral("Done. %1/%2 succeeded.").arg(total - failures).arg(total));
    return failures == 0 ? 0 : 1;
}

void printHelp(const Options& opts)
{
    printOutLine(opts.helpText.isEmpty() ? QStringLiteral("Use --help for usage.") : opts.helpText);
}

void printVersion(const Options& opts)
{
    printOutLine(opts.versionText);
}

void printFormats()
{
    const auto& reg = FormatRegistry::instance();
    auto printCat = [&](const QString& title, const QStringList& items) {
        QString line = QStringLiteral("%1 (%2): ").arg(title).arg(items.size());
        for (int i = 0; i < items.size(); ++i)
        {
            line += items[i];
            if (i + 1 < items.size())
                line += QStringLiteral(", ");
        }
        printOutLine(line);
    };
    printCat("Video", reg.videoFormats());
    printCat("Audio", reg.audioFormats());
    printCat("Image input", reg.imageInputFormats());
    printCat("Image output", reg.imageOutputFormats());
    printCat("Document input", reg.documentInputFormats());
    printCat("Document output", reg.documentOutputFormats());
}

} // namespace CliRunner
