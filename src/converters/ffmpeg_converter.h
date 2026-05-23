#ifndef FFMPEG_CONVERTER_H
#define FFMPEG_CONVERTER_H

#include "iconverter.h"
#include "error_types.h"
#include <QProcess>
#include <QTimer>
#include <QMap>
#include <QRegularExpression>

class FFmpegConverter : public QObject, public IConverter {
    Q_OBJECT

public:
    explicit FFmpegConverter(QObject* parent = nullptr);
    ~FFmpegConverter() override;

    bool convert(const QString& inputFile, const QString& outputFile,
                const QVariantMap& params) override;
    QStringList supportedInputFormats() const override;
    QStringList supportedOutputFormats() const override;
    QString name() const override { return QStringLiteral("FFmpeg"); }
    bool isConversionSupported(const QString& inputFormat,
                              const QString& outputFormat) const override;

    void setFFmpegPath(const QString& path) { m_ffmpegPath = path; }
    QString ffmpegPath() const { return m_ffmpegPath; }
    void setFFprobePath(const QString& path) { m_ffprobePath = path; }
    QString ffprobePath() const { return m_ffprobePath; }
    bool isRunning() const { return m_isRunning; }
    void cancel();

    bool extractAudio(const QString& inputFile, const QString& outputFile,
                     const QVariantMap& params = QVariantMap());
    bool getMediaInfo(const QString& filePath, QVariantMap& info);
    double getDuration(const QString& filePath);

    double currentSpeed() const { return m_currentSpeed; }
    qint64 estimatedRemainingMs() const { return m_estimatedRemainingMs; }
    double currentBitrate() const { return m_currentBitrate; }
    qint64 processedBytes() const { return m_processedBytes; }

    static QMap<QString, QString> videoFormatMap();
    static QMap<QString, QString> audioFormatMap();
    static QMap<QString, QString> videoCodecMap();
    static QMap<QString, QString> audioCodecMap();

signals:
    void progressChanged(int progress);
    void statusChanged(const QString& status);
    void conversionFinished(bool success, const QString& message);
    void errorOccurred(const ErrorInfo& error);
    void detailedProgress(int progress, double speed, qint64 remainingMs, double bitrate);

private slots:
    void onProcessReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    bool runFFmpeg(const QStringList& args);
    bool runFFprobe(const QStringList& args, QString& output);
    QString getFormatFromExtension(const QString& filePath) const;
    QStringList buildVideoArgs(const QVariantMap& params);
    QStringList buildAudioArgs(const QVariantMap& params);
    void parseProgress(const QString& line);
    int parseTimeToMs(const QString& timeStr);
    bool isVideoFormat(const QString& format) const;
    bool isAudioFormat(const QString& format) const;
    void updateEstimatedTime(int currentProgress);

    QString m_ffmpegPath;
    QString m_ffprobePath;
    QStringList m_videoFormats;
    QStringList m_audioFormats;
    QProcess* m_process;
    QTimer* m_timeoutTimer;
    bool m_isRunning;
    double m_totalDuration;
    QString m_currentOutputFile;
    QString m_currentInputFile;
    QString m_errorBuffer;
    QRegularExpression m_timeRegex;
    QRegularExpression m_progressRegex;
    QRegularExpression m_speedRegex;
    QRegularExpression m_bitrateRegex;
    QRegularExpression m_sizeRegex;
    ErrorInfo m_lastError;
    double m_currentSpeed;
    qint64 m_estimatedRemainingMs;
    double m_currentBitrate;
    qint64 m_processedBytes;
    qint64 m_conversionStartTime;
    QList<QPair<int, qint64>> m_progressHistory;
};
#endif // FFMPEG_CONVERTER_H
