#include "conversion_planner.h"

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QStringList>

namespace ConversionPlanner
{

int parseBitrateToKbps(const QString& bitrateStr)
{
    if (bitrateStr.isEmpty())
        return 0;
    QString str = bitrateStr.trimmed().toLower();
    if (str == "auto")
        return 0;
    if (str.endsWith("k"))
    {
        bool ok;
        int val = str.left(str.length() - 1).toInt(&ok);
        return ok ? val : 0;
    }
    if (str.endsWith("m"))
    {
        bool ok;
        int val = str.left(str.length() - 1).toInt(&ok);
        return ok ? val * 1000 : 0;
    }
    bool ok;
    int val = str.toInt(&ok);
    return ok ? val : 0;
}

QString outputPath(const QString& input, const QString& explicitOutput, const QString& outputDir, const QString& format)
{
    if (!explicitOutput.isEmpty())
    {
        return explicitOutput;
    }
    QFileInfo info(input);
    QString ext = format.startsWith('.') ? format.mid(1) : format;
    if (ext.isEmpty())
    {
        ext = info.suffix();
    }
    QString outPath = QDir(outputDir).filePath(info.completeBaseName() + QStringLiteral(".") + ext);
    // Never clobber the source: same resolved path -> "_converted" suffix.
    if (QFileInfo(outPath).absoluteFilePath() == info.absoluteFilePath())
    {
        outPath = QDir(outputDir).filePath(info.completeBaseName() + QStringLiteral("_converted.") + ext);
    }
    return outPath;
}

QString converterNameFor(const QString& outputExt, const QString& inputExt)
{
    const auto& reg = FormatRegistry::instance();
    auto pick = reg.converterForExt(outputExt);
    if (pick == FormatRegistry::Converter::Unknown)
    {
        pick = reg.converterForExt(inputExt);
    }
    switch (pick)
    {
        case FormatRegistry::Converter::FFmpeg:
            return QStringLiteral("FFmpeg");
        case FormatRegistry::Converter::Pandoc:
            return QStringLiteral("Pandoc");
        case FormatRegistry::Converter::ImageMagick:
            return QStringLiteral("ImageMagick");
        default:
            return QString();
    }
}

QVariantMap mergeParams(const QVariantMap& baseParams, const QVariantMap& dialogParams, FormatRegistry::Category cat)
{
    QVariantMap merged = baseParams;

    for (auto it = dialogParams.begin(); it != dialogParams.end(); ++it)
    {
        const QString& key = it.key();
        const QVariant& value = it.value();

        // Skip empty/default values to avoid overwriting base params
        if (!value.isValid())
            continue;

        if (cat == FormatRegistry::Category::Audio || cat == FormatRegistry::Category::Video)
        {
            // Normalize resolution to lowercase (FFmpeg requires lowercase 'x')
            if (key == "resolution")
            {
                QString res = value.toString().trimmed().toLower();
                if (!res.isEmpty())
                {
                    merged["resolution"] = res;
                }
                continue;
            }
            if (key == "videoBitrate" || key == "audioBitrate")
            {
                QString bs = value.toString();
                if (bs.isEmpty())
                    continue;
                int kbps = parseBitrateToKbps(bs);
                if (kbps > 0)
                {
                    merged[key] = kbps;
                }
                continue;
            }
            if (key == "framerate")
            {
                QString fps = value.toString();
                if (!fps.isEmpty())
                {
                    bool ok;
                    int fpsInt = fps.toInt(&ok);
                    if (ok)
                    {
                        merged["frameRate"] = fpsInt;
                    }
                }
                continue;
            }
            if (key == "twoPass")
            {
                merged["twoPass"] = value;
                continue;
            }
        }

        if (cat == FormatRegistry::Category::Document)
        {
            if (key == "pageSize" || key == "orientation" || key == "marginTop" || key == "marginBottom" ||
                key == "marginLeft" || key == "marginRight")
            {
                continue; // geometry vars assembled below
            }
        }

        // Pass through all other values
        merged[key] = value;
    }

    // Build pandoc geometry variables
    if (cat == FormatRegistry::Category::Document)
    {
        QStringList geoParts;
        QString pageSize = dialogParams.value("pageSize").toString();
        if (!pageSize.isEmpty() && pageSize != "custom")
        {
            geoParts << pageSize;
        }
        QString orientation = dialogParams.value("orientation").toString();
        if (orientation == "landscape")
        {
            geoParts << "landscape";
        }
        auto addMargin = [&](const QString& key, const QString& side) {
            double val = dialogParams.value(key, 1.0).toDouble();
            if (val > 0)
            {
                geoParts << QString("%1=%2in").arg(side).arg(val, 0, 'f', 1);
            }
        };
        addMargin("marginTop", "top");
        addMargin("marginBottom", "bottom");
        addMargin("marginLeft", "left");
        addMargin("marginRight", "right");

        if (!geoParts.isEmpty())
        {
            QVariantMap varMap = merged.value("variableMap").toMap();
            varMap["geometry"] = geoParts.join(",");
            merged["variableMap"] = varMap;
        }

        if (dialogParams.value("numberSections", false).toBool())
        {
            QStringList extraArgs = merged.value("extraArgs").toStringList();
            extraArgs << "--number-sections";
            merged["extraArgs"] = extraArgs;
        }
    }

    return merged;
}

} // namespace ConversionPlanner
