#ifndef ICONVERTER_H
#define ICONVERTER_H
#include <QString>
#include <QStringList>
#include <QVariantMap>
class IConverter {
public:
    virtual ~IConverter() = default;
    virtual bool convert(const QString& inputFile, const QString& outputFile,
                        const QVariantMap& params) = 0;
    virtual QStringList supportedInputFormats() const = 0;
    virtual QStringList supportedOutputFormats() const = 0;
    virtual QString name() const = 0;
    virtual bool isConversionSupported(const QString& inputFormat,
                                       const QString& outputFormat) const = 0;
};
#endif // ICONVERTER_H
