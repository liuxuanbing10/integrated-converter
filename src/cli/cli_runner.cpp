#include "cli_runner.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QTextStream>

#include "core/format_registry.h"
#include "core/iconverter.h"
#include "core/logger.h"

#include <cstdio>

namespace CliRunner {

namespace {

void printOut(const QString& s) {
    const QByteArray utf8 = s.toUtf8();
    fwrite(utf8.constData(), 1, utf8.size(), stdout);
    fflush(stdout);
}

void printErr(const QString& s) {
    const QByteArray utf8 = s.toUtf8();
    fwrite(utf8.constData(), 1, utf8.size(), stderr);
    fflush(stderr);
}

void printOutLine(const QString& s) {
    printOut(s + "\n");
}

QString resolveOutputPath(const QString& input,
                          const QString& explicitOutput,
                          const QString& outputDir,
                          const QString& format) {
    if (!explicitOutput.isEmpty()) {
        return explicitOutput;
    }
    QFileInfo info(input);
    QString ext = format.startsWith('.') ? format.mid(1) : format;
    if (ext.isEmpty()) {
        ext = info.suffix();
    }
    QString baseName = info.completeBaseName();
    QString outPath = QDir(outputDir).filePath(baseName + QStringLiteral(".") + ext);
    return outPath;
}

} // namespace

Options parseArgs(const QStringList& args, QString* errorMessage) {
    Options opts;
    auto fail = [&](const QString& msg) {
        if (errorMessage) *errorMessage = msg;
    };

    for (int i = 0; i < args.size(); ++i) {
        const QString& a = args[i];
        if (a == "--help" || a == "-h") {
            opts.showHelp = true;
        } else if (a == "--list-formats") {
            opts.listFormats = true;
        } else if (a == "--verbose" || a == "-v") {
            opts.verbose = true;
        } else if (a == "--input" || a == "-i") {
            if (i + 1 >= args.size()) { fail("--input requires a path"); return opts; }
            opts.inputs << args[++i];
        } else if (a == "--output" || a == "-o") {
            if (i + 1 >= args.size()) { fail("--output requires a path"); return opts; }
            opts.outputs << args[++i];
        } else if (a == "--output-dir") {
            if (i + 1 >= args.size()) { fail("--output-dir requires a path"); return opts; }
            opts.outputDir = args[++i];
        } else if (a == "--format" || a == "-f") {
            if (i + 1 >= args.size()) { fail("--format requires an extension"); return opts; }
            opts.format = args[++i].toLower();
            if (opts.format.startsWith('.')) opts.format = opts.format.mid(1);
        } else if (a == "--codec") {
            if (i + 1 >= args.size()) { fail("--codec requires a name"); return opts; }
            opts.conversionParams["videoCodec"] = args[++i];
        } else if (a == "--audio-codec") {
            if (i + 1 >= args.size()) { fail("--audio-codec requires a name"); return opts; }
            opts.conversionParams["audioCodec"] = args[++i];
        } else if (a == "--crf") {
            if (i + 1 >= args.size()) { fail("--crf requires an int"); return opts; }
            opts.conversionParams["crf"] = args[++i].toInt();
        } else if (a == "--bitrate") {
            if (i + 1 >= args.size()) { fail("--bitrate requires kbps"); return opts; }
            opts.conversionParams["videoBitrate"] = args[++i].toInt();
        } else if (a == "--audio-bitrate") {
            if (i + 1 >= args.size()) { fail("--audio-bitrate requires kbps"); return opts; }
            opts.conversionParams["audioBitrate"] = args[++i].toInt();
        } else if (a == "--preset") {
            if (i + 1 >= args.size()) { fail("--preset requires a name"); return opts; }
            opts.conversionParams["preset"] = args[++i];
        } else if (a == "--resolution" || a == "-s") {
            if (i + 1 >= args.size()) { fail("--resolution requires WxH"); return opts; }
            opts.conversionParams["resolution"] = args[++i];
        } else if (a.startsWith("--")) {
            fail(QStringLiteral("Unknown option: %1").arg(a));
            return opts;
        } else {
            // Positional argument: treat as another --input.
            opts.inputs << a;
        }
    }

    if (!opts.showHelp && !opts.listFormats) {
        if (opts.inputs.isEmpty()) {
            fail("No input files. Use --input <path> or pass files positionally.");
        } else if (opts.outputs.isEmpty() && opts.outputDir.isEmpty()) {
            fail("Either --output (one per input) or --output-dir + --format is required.");
        } else if (!opts.outputs.isEmpty() && opts.outputs.size() != opts.inputs.size()) {
            fail(QStringLiteral("--output count (%1) must match --input count (%2).")
                     .arg(opts.outputs.size()).arg(opts.inputs.size()));
        } else if (!opts.outputDir.isEmpty() && opts.format.isEmpty()) {
            fail("--output-dir requires --format <ext>.");
        }
    }
    return opts;
}

int run(const Options& opts,
        const QHash<QString, void*>& convertersByName,
        QString* errorMessage) {
    if (opts.inputs.isEmpty()) {
        if (errorMessage) *errorMessage = "No input files.";
        return 1;
    }
    const auto& reg = FormatRegistry::instance();
    int failures = 0;
    int total = opts.inputs.size();

    for (int idx = 0; idx < total; ++idx) {
        const QString& input = opts.inputs[idx];
        QString output = resolveOutputPath(input,
                                           idx < opts.outputs.size() ? opts.outputs[idx] : QString(),
                                           opts.outputDir,
                                           opts.format);
        QFileInfo inInfo(input);
        if (!inInfo.exists()) {
            printErr(QStringLiteral("[skip] %1: file does not exist\n").arg(input));
            ++failures;
            continue;
        }
        QString converterName;
        FormatRegistry::Converter picked = reg.converterForExt(QFileInfo(output).suffix().toLower());
        if (picked == FormatRegistry::Converter::Unknown) {
            picked = reg.converterForExt(inInfo.suffix().toLower());
        }
        switch (picked) {
        case FormatRegistry::Converter::FFmpeg:      converterName = "FFmpeg"; break;
        case FormatRegistry::Converter::Pandoc:      converterName = "Pandoc"; break;
        case FormatRegistry::Converter::ImageMagick: converterName = "ImageMagick"; break;
        default:
            printErr(QStringLiteral("[skip] %1: no converter for format\n").arg(input));
            ++failures;
            continue;
        }
        auto it = convertersByName.find(converterName);
        if (it == convertersByName.end()) {
            printErr(QStringLiteral("[skip] %1: converter %2 not registered\n")
                         .arg(input, converterName));
            ++failures;
            continue;
        }
        auto* converter = static_cast<IConverter*>(it.value());

        printOut(QStringLiteral("[%1/%2] %3 -> %4 (%5)")
                     .arg(idx + 1).arg(total).arg(input, output, converterName));

        // Ensure output directory exists.
        QFileInfo outInfo(output);
        if (!outInfo.absoluteDir().exists()) {
            QDir().mkpath(outInfo.absolutePath());
        }

        auto result = converter->convert(input, output, opts.conversionParams);
        if (!result.has_value()) {
            printOutLine(QStringLiteral("  [ok]"));
            LOG_INFO("CLI", QString("OK: %1 -> %2").arg(input, output));
        } else {
            QString msg = result->fullMessage();
            printOutLine(QStringLiteral("  [FAILED] %1").arg(msg));
            LOG_ERROR("CLI", QString("FAIL: %1 -> %2 - %3").arg(input, output, msg));
            ++failures;
        }
    }
    printOutLine(QStringLiteral("Done. %1/%2 succeeded.").arg(total - failures).arg(total));
    return failures == 0 ? 0 : 1;
}

void printHelp() {
    printOut(
        "Integrated Format Converter - CLI mode\n"
        "Usage: integrated_converter --cli [options]\n"
        "\n"
        "Options:\n"
        "  -i, --input <path>         Input file (repeatable, or pass positionally)\n"
        "  -o, --output <path>        Output file (repeatable, must match --input count)\n"
        "      --output-dir <dir>     Directory to write outputs to (use with --format)\n"
        "  -f, --format <ext>         Target format/extension (e.g. mp3, mp4, png)\n"
        "      --codec <name>         Video codec (e.g. libx264, libx265, libvpx-vp9)\n"
        "      --audio-codec <name>   Audio codec (e.g. aac, libmp3lame, libopus)\n"
        "      --crf <int>            Constant rate factor (0-51, lower = better)\n"
        "      --bitrate <kbps>       Video bitrate\n"
        "      --audio-bitrate <kbps> Audio bitrate\n"
        "      --preset <name>        Encoder preset (ultrafast..veryslow)\n"
        "  -s, --resolution <WxH>    Output resolution (e.g. 1920x1080)\n"
        "      --list-formats         Print all supported formats and exit\n"
        "  -v, --verbose              Verbose logging to stderr\n"
        "  -h, --help                 Show this help\n"
        "\n"
        "Examples:\n"
        "  integrated_converter --cli -i in.mp4 -o out.mp3\n"
        "  integrated_converter --cli -i in.mov -o out.mp4 --codec libx265 --crf 28\n"
        "  integrated_converter --cli -i *.png --output-dir ./out --format webp\n"
        "\n");
}

void printFormats() {
    const auto& reg = FormatRegistry::instance();
    auto printCat = [&](const QString& title, const QStringList& items) {
        QString line = QStringLiteral("%1 (%2): ").arg(title).arg(items.size());
        for (int i = 0; i < items.size(); ++i) {
            line += items[i];
            if (i + 1 < items.size()) line += QStringLiteral(", ");
        }
        printOutLine(line);
    };
    printCat("Video", reg.videoFormats());
    printCat("Audio", reg.audioFormats());
    printCat("Image input",  reg.imageInputFormats());
    printCat("Image output", reg.imageOutputFormats());
    printCat("Document input",  reg.documentInputFormats());
    printCat("Document output", reg.documentOutputFormats());
}

} // namespace CliRunner
