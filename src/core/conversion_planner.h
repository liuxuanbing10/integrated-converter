#ifndef CONVERSION_PLANNER_H
#define CONVERSION_PLANNER_H

#include "format_registry.h"

#include <QString>
#include <QVariantMap>

/// Pure planning logic shared by the GUI submit flow and the CLI runner
/// (analysis §4.2: this orchestration used to be copy-pasted in
/// main_window.cpp and diverged from cli_runner.cpp).
///
/// What belongs here: everything that can be decided WITHOUT a widget or a
/// process — output-path naming (incl. the overwrite-source rename),
/// converter routing by extension, and normalization of dialog params.
namespace ConversionPlanner
{

/// "500k" / "1M" / "2000" -> kbps int; 0 for empty/auto/unparseable.
int parseBitrateToKbps(const QString& bitrateStr);

/// Resolve the output path for `input`.
///  - explicitOutput wins verbatim (CLI -o flow).
///  - otherwise dir/<completeBaseName>.<format>; empty format keeps the
///    source extension. If the result equals the input path, the basename
///    gets a "_converted" suffix so we never overwrite the source.
QString outputPath(const QString& input, const QString& explicitOutput, const QString& outputDir,
                   const QString& format);

/// Route an extension to its converter name ("FFmpeg"/"Pandoc"/"ImageMagick"),
/// preferring the OUTPUT format and falling back to the input's own suffix
/// (the same order the CLI used before extraction). Returns an empty string
/// when nothing can handle the pair.
QString converterNameFor(const QString& outputExt, const QString& inputExt);

/// Merge saved dialog params over a base task-params map, normalizing keys
/// per category: lowercase WxH resolution, bitrate strings -> kbps ints,
/// framerate -> frameRate, twoPass passthrough, pandoc geometry variable
/// assembly and --number-sections. Pure; pass cat::Unknown to skip
/// category-specific fixups.
QVariantMap mergeParams(const QVariantMap& baseParams, const QVariantMap& dialogParams, FormatRegistry::Category cat);

} // namespace ConversionPlanner

#endif // CONVERSION_PLANNER_H
