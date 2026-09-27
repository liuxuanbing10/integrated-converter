#ifndef PANDOC_CONVERTER_H
#define PANDOC_CONVERTER_H
#include "error_types.h"
#include "iconverter.h"

#include <QProcess>
#include <QSet>

#include <atomic>
#include <memory>
class PandocConverter : public QObject, public IConverter
{
    Q_OBJECT
public:
    explicit PandocConverter(QObject* parent = nullptr);
    // Cloneable: one independent instance per concurrent task (see IConverter).
    PandocConverter(const PandocConverter& other);
    std::unique_ptr<IConverter> clone() const override;
    ~PandocConverter() override;
    std::optional<ErrorInfo> convert(const QString& inputFile, const QString& outputFile,
                                     const QVariantMap& params) override;
    QStringList supportedInputFormats() const override;
    QStringList supportedOutputFormats() const override;
    QString name() const override
    {
        return QStringLiteral("Pandoc");
    }
    bool isConversionSupported(const QString& inputFormat, const QString& outputFormat) const override;
    void setPandocPath(const QString& path)
    {
        m_pandocPath = path;
    }
    QString pandocPath() const
    {
        return m_pandocPath;
    }
    QString getPandocFormat(const QString& format) const;
    bool checkPandocAvailable() const;
    QString getPandocVersion() const;
    void cancel();
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
    bool runPandoc(const QStringList& args, QString& output);
    QStringList buildArguments(const QString& inputFile, const QString& outputFile, const QVariantMap& params);
    QString detectInputFormat(const QString& filePath) const;
    QString detectOutputFormat(const QString& filePath) const;
    QString m_pandocPath;
    QSet<QString> m_inputFormats;
    QSet<QString> m_outputFormats;
    std::unique_ptr<QProcess> m_currentProcess;
    QString m_currentInputFile;
    QString m_currentOutputFile;
    std::atomic<bool> m_isConverting;
    ErrorInfo m_lastError;
};
#endif // PANDOC_CONVERTER_H
