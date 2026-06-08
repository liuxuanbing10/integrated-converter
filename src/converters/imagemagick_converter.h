#ifndef IMAGEMAGICK_CONVERTER_H
#define IMAGEMAGICK_CONVERTER_H

#include "iconverter.h"
#include "error_types.h"
#include <QProcess>
#include <memory>

class ImageMagickConverter : public QObject, public IConverter {
    Q_OBJECT

public:
    explicit ImageMagickConverter(QObject* parent = nullptr);
    ~ImageMagickConverter() override;

    std::optional<ErrorInfo> convert(const QString& inputFile, const QString& outputFile,
                                      const QVariantMap& params) override;
    QStringList supportedInputFormats() const override;
    QStringList supportedOutputFormats() const override;
    QString name() const override { return QStringLiteral("ImageMagick"); }
    bool isConversionSupported(const QString& inputFormat,
                               const QString& outputFormat) const override;

    void setMagickPath(const QString& path) { m_magickPath = path; }
    QString magickPath() const { return m_magickPath; }
    bool isRunning() const { return m_isRunning; }
    void cancel();

    bool identify(const QString& filePath, QVariantMap& info);
    bool validateParams(const QVariantMap& params, QString& errorMsg);

    double currentProgress() const { return m_currentProgress; }

signals:
    void progressChanged(int progress);
    void statusChanged(const QString& status);
    void conversionFinished(bool success, const QString& message);
    void errorOccurred(const ErrorInfo& error);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onProcessReadyReadStandardError();

private:
    bool runMagick(const QStringList& args);
    QStringList buildArguments(const QString& inputFile, const QString& outputFile,
                                const QVariantMap& params) const;
    QString getFormatFromExtension(const QString& filePath) const;

    QString m_magickPath;
    QStringList m_inputFormats;
    QStringList m_outputFormats;
    std::unique_ptr<QProcess> m_process;
    bool m_isRunning;
    QString m_currentInputFile;
    QString m_currentOutputFile;
    QString m_errorBuffer;
    ErrorInfo m_lastError;
    double m_currentProgress;
};

#endif // IMAGEMAGICK_CONVERTER_H
