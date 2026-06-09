#include "imagemagick_converter.h"
#include "config_manager.h"
#include "format_registry.h"
#include "logger.h"
#include "error_handler.h"
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QRegularExpression>

static constexpr qint64 WAIT_FOR_FINISHED_TIMEOUT_MS = 300000; // 5 min

ImageMagickConverter::ImageMagickConverter(QObject* parent)
    : QObject(parent)
    , m_magickPath("magick")
    , m_process(nullptr)
    , m_isRunning(false)
    , m_currentProgress(0.0)
{
    m_magickPath = ConfigManager::instance().value("imagemagickPath", "magick").toString();
    const auto& reg = FormatRegistry::instance();
    m_inputFormats = QSet<QString>(reg.imageInputFormats().begin(), reg.imageInputFormats().end());
    m_outputFormats = QSet<QString>(reg.imageOutputFormats().begin(), reg.imageOutputFormats().end());
}

ImageMagickConverter::~ImageMagickConverter() {
    cancel();
}

QStringList ImageMagickConverter::supportedInputFormats() const {
    return QStringList(m_inputFormats.cbegin(), m_inputFormats.cend());
}

QStringList ImageMagickConverter::supportedOutputFormats() const {
    return QStringList(m_outputFormats.cbegin(), m_outputFormats.cend());
}

bool ImageMagickConverter::isConversionSupported(const QString& inputFormat,
                                                  const QString& outputFormat) const {
    QString input = inputFormat.toLower();
    QString output = outputFormat.toLower();
    bool inputSupported = m_inputFormats.contains(input);
    bool outputSupported = m_outputFormats.contains(output);
    // ImageMagick can convert between any of its supported formats
    return inputSupported && outputSupported;
}

QString ImageMagickConverter::getFormatFromExtension(const QString& filePath) const {
    return QFileInfo(filePath).suffix().toLower();
}

QStringList ImageMagickConverter::buildArguments(const QString& inputFile,
                                                  const QString& outputFile,
                                                  const QVariantMap& params) const {
    QStringList args;

    // Input file
    args << inputFile;

    // Enable progress monitoring via stderr output
    args << "-monitor";

    // Optional resize
    QString resize = params.value("resize").toString();
    if (!resize.isEmpty()) {
        args << "-resize" << resize;
    }

    // Quality (1-100)
    int quality = params.value("quality", 0).toInt();
    if (quality > 0 && quality <= 100) {
        args << "-quality" << QString::number(quality);
    }

    // Compression type
    QString compression = params.value("compression").toString();
    if (!compression.isEmpty()) {
        args << "-compress" << compression;
    }

    // Density (DPI)
    int density = params.value("density", 0).toInt();
    if (density > 0) {
        args << "-density" << QString::number(density);
    }

    // Depth (bit depth)
    QString depth = params.value("depth").toString();
    if (!depth.isEmpty()) {
        args << "-depth" << depth;
    }

    // Strip metadata
    bool strip = params.value("strip", false).toBool();
    if (strip) {
        args << "-strip";
    }

    // Output file
    args << outputFile;

    return args;
}

std::optional<ErrorInfo> ImageMagickConverter::convert(const QString& inputFile, const QString& outputFile,
                                                        const QVariantMap& params) {
    LOG_INFO("ImageMagick", QString("开始转换: %1 -> %2").arg(inputFile, outputFile));

    QFileInfo inputInfo(inputFile);
    if (!inputInfo.exists()) {
        ErrorInfo error = ErrorTypes::createFileNotFoundError(inputFile, "ImageMagick::convert");
        error.outputFile = outputFile;
        m_lastError = error;
        LOG_ERROR("ImageMagick", QString("输入文件不存在: %1").arg(inputFile));
        ErrorHandler::instance()->handleError(error);
        emit errorOccurred(error);
        emit conversionFinished(false, error.message);
        return error;
    }

    // Validate parameters
    QString paramError;
    if (!validateParams(params, paramError)) {
        ErrorInfo error = ErrorTypes::createError(
            ErrorCode::InvalidParameter, paramError, "ImageMagick::convert");
        error.inputFile = inputFile;
        error.outputFile = outputFile;
        m_lastError = error;
        LOG_ERROR("ImageMagick", paramError);
        ErrorHandler::instance()->handleError(error);
        emit errorOccurred(error);
        emit conversionFinished(false, paramError);
        return error;
    }

    m_currentInputFile = inputFile;
    m_currentOutputFile = outputFile;
    m_errorBuffer.clear();
    m_currentProgress = 0.0;

    QStringList args = buildArguments(inputFile, outputFile, params);
    if (runMagick(args)) return std::nullopt;
    if (m_lastError.isValid()) return m_lastError;
    return ErrorTypes::createConversionFailedError(
        tr("ImageMagick 转换失败"), "ImageMagick", "ImageMagick::convert");
}

bool ImageMagickConverter::runMagick(const QStringList& args) {
    if (m_isRunning) {
        LOG_ERROR("ImageMagick", tr("已有转换任务在运行"));
        return false;
    }

    m_process = std::make_unique<QProcess>();
    m_process->setProgram(m_magickPath);
    m_process->setArguments(args);

    connect(m_process.get(), &QProcess::readyReadStandardError,
            this, &ImageMagickConverter::onProcessReadyReadStandardError);
    connect(m_process.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ImageMagickConverter::onProcessFinished);
    connect(m_process.get(), &QProcess::errorOccurred,
            this, &ImageMagickConverter::onProcessError);

    LOG_DEBUG("ImageMagick", QString("执行命令: %1 %2").arg(m_magickPath, args.join(" ")));

    m_isRunning = true;
    emit statusChanged(tr("正在启动ImageMagick..."));
    m_process->start();

    if (!m_process->waitForStarted(5000)) {
        m_isRunning = false;
        ErrorInfo error = ErrorTypes::createProcessError(
            ErrorCode::ProcessFailedToStart, "ImageMagick",
            m_process->errorString(), "ImageMagick::runMagick");
        error.inputFile = m_currentInputFile;
        error.outputFile = m_currentOutputFile;
        m_lastError = error;
        LOG_ERROR("ImageMagick", error.message);
        ErrorHandler::instance()->handleError(error);
        emit errorOccurred(error);
        emit conversionFinished(false, error.message);
        return false;
    }

    emit statusChanged(tr("正在转换..."));
    emit progressChanged(50);

    if (!m_process->waitForFinished(WAIT_FOR_FINISHED_TIMEOUT_MS)) {
        LOG_WARNING("ImageMagick", tr("ImageMagick 进程超时，正在终止"));
        m_process->kill();
        m_process->waitForFinished(5000);
        m_isRunning = false;
        ErrorInfo error = ErrorTypes::createError(
            ErrorCode::TaskTimeout,
            tr("ImageMagick 转换超时"),
            "ImageMagick::runMagick");
        error.inputFile = m_currentInputFile;
        error.outputFile = m_currentOutputFile;
        m_lastError = error;
        LOG_ERROR("ImageMagick", error.message);
        ErrorHandler::instance()->handleError(error);
        emit errorOccurred(error);
        emit conversionFinished(false, error.message);
        return false;
    }

    bool success = m_process->exitStatus() == QProcess::NormalExit && m_process->exitCode() == 0;
    m_isRunning = false;
    // Destroy the finished QProcess to close OS pipe handles and prevent stale
    // queued signal events (readyReadStandardError, finished) from firing during
    // the nested event loop of QDialog::exec() in the completion flow.
    m_process.reset();
    return success;
}

void ImageMagickConverter::cancel() {
    if (m_isRunning && m_process) {
        LOG_INFO("ImageMagick", tr("取消转换任务"));
        m_process->kill();
        m_process->waitForFinished(3000);
        m_process.reset();
        m_isRunning = false;
        emit statusChanged(tr("已取消"));
        emit conversionFinished(false, tr("用户取消"));
    }
}

bool ImageMagickConverter::identify(const QString& filePath, QVariantMap& info) {
    QProcess process;
    process.setProgram(m_magickPath);
    process.setArguments(QStringList() << "identify" << "-verbose" << filePath);
    process.start();

    if (!process.waitForStarted(5000)) {
        return false;
    }
    if (!process.waitForFinished(30000)) {
        return false;
    }
    if (process.exitCode() != 0) {
        return false;
    }

    QString output = QString::fromUtf8(process.readAllStandardOutput());
    QRegularExpression geomRe(R"(Geometry:\s+(\d+)x(\d+))");
    QRegularExpressionMatch geomMatch = geomRe.match(output);
    if (geomMatch.hasMatch()) {
        info["width"] = geomMatch.captured(1).toInt();
        info["height"] = geomMatch.captured(2).toInt();
    }

    QRegularExpression formatRe(R"(Format:\s+(\w+))");
    QRegularExpressionMatch formatMatch = formatRe.match(output);
    if (formatMatch.hasMatch()) {
        info["format"] = formatMatch.captured(1);
    }

    QRegularExpression depthRe(R"(Depth:\s+(\d+))");
    QRegularExpressionMatch depthMatch = depthRe.match(output);
    if (depthMatch.hasMatch()) {
        info["depth"] = depthMatch.captured(1).toInt();
    }

    QRegularExpression filesizeRe(R"(Filesize:\s+([\d.]+[A-Z]?))");
    QRegularExpressionMatch filesizeMatch = filesizeRe.match(output);
    if (filesizeMatch.hasMatch()) {
        info["fileSize"] = filesizeMatch.captured(1);
    }

    return true;
}

bool ImageMagickConverter::validateParams(const QVariantMap& params, QString& errorMsg) {
    // Validate resize format (e.g., "800x600", "50%", "1920x1080!")
    QString resize = params.value("resize").toString();
    if (!resize.isEmpty()) {
        static const QRegularExpression resizeRe(R"(^\d+[xX]\d+!?$|^\d+%$|^x\d+$|^\d+$)");
        if (!resizeRe.match(resize).hasMatch()) {
            errorMsg = QString("无效的缩放参数: %1 (期望格式如 800x600, 50%%, x1080, 800)").arg(resize);
            return false;
        }
    }

    // Validate quality
    int quality = params.value("quality", 0).toInt();
    if (quality < 0 || quality > 100) {
        errorMsg = QString("质量参数超出范围: %1 (有效范围: 0-100)").arg(quality);
        return false;
    }

    // Validate density
    int density = params.value("density", 0).toInt();
    if (density < 0) {
        errorMsg = QString("DPI不能为负数: %1").arg(density);
        return false;
    }
    if (density > 12000) {
        errorMsg = QString("DPI过高: %1 (上限: 12000)").arg(density);
        return false;
    }

    return true;
}

void ImageMagickConverter::onProcessReadyReadStandardError() {
    if (!m_process) return;
    QString output = QString::fromUtf8(m_process->readAllStandardError());
    m_errorBuffer += output;

    // Try to parse progress from ImageMagick output
    // ImageMagick outputs progress lines like: "filename.jpg 15.2% 10.1M"
    static const QRegularExpression progressRe(R"(([\d.]+)%\s+)");
    QRegularExpressionMatch match = progressRe.match(output);
    if (match.hasMatch()) {
        double pct = match.captured(1).toDouble();
        m_currentProgress = pct;
        emit progressChanged(static_cast<int>(pct));
    }
}

void ImageMagickConverter::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    // Guard: if already marked not running (e.g. cancel() already handled this),
    // do nothing to avoid double-emission of completion signals.
    if (!m_isRunning) return;
    m_isRunning = false;
    bool success = (exitCode == 0 && exitStatus == QProcess::NormalExit);

    if (success) {
        LOG_INFO("ImageMagick", QString("转换完成: %1").arg(m_currentOutputFile));
        emit progressChanged(100);
        emit statusChanged(tr("转换完成"));
        emit conversionFinished(true, tr("转换成功"));
    } else {
        ErrorInfo error;
        if (exitStatus == QProcess::CrashExit) {
            error = ErrorTypes::createProcessError(ErrorCode::ProcessCrashed, "ImageMagick",
                                                   QString(), "ImageMagick::onProcessFinished");
        } else {
            QString detailMsg;
            if (!m_errorBuffer.isEmpty()) {
                QStringList lines = m_errorBuffer.split('\n', Qt::SkipEmptyParts);
                for (int i = lines.size() - 1; i >= 0 && detailMsg.length() < 500; --i) {
                    if (lines[i].contains("Error") || lines[i].contains("error") ||
                        lines[i].contains("Invalid") || lines[i].contains("failed")) {
                        detailMsg += lines[i].trimmed() + "\n";
                    }
                }
            }
            error = ErrorTypes::createConversionFailedError(
                detailMsg.trimmed().isEmpty()
                    ? tr("退出码: %1").arg(exitCode)
                    : detailMsg.trimmed(),
                "ImageMagick", "ImageMagick::onProcessFinished");
        }
        error.inputFile = m_currentInputFile;
        error.outputFile = m_currentOutputFile;
        m_lastError = error;
        LOG_ERROR("ImageMagick", error.fullMessage());
        ErrorHandler::instance()->handleError(error);
        emit errorOccurred(error);
        emit statusChanged(tr("转换失败"));
        emit conversionFinished(false, error.message);
    }
}

void ImageMagickConverter::onProcessError(QProcess::ProcessError error) {
    // Guard: if already handled, skip to avoid double-emission
    if (!m_isRunning) return;
    m_isRunning = false;
    ErrorCode errorCode;
    switch (error) {
        case QProcess::FailedToStart:
            errorCode = ErrorCode::ProcessFailedToStart;
            break;
        case QProcess::Crashed:
            errorCode = ErrorCode::ProcessCrashed;
            break;
        case QProcess::Timedout:
            errorCode = ErrorCode::TaskTimeout;
            break;
        default:
            errorCode = ErrorCode::Unknown;
            break;
    }
    ErrorInfo err = ErrorTypes::createProcessError(errorCode, "ImageMagick",
                                                   QString(), "ImageMagick::onProcessError");
    err.inputFile = m_currentInputFile;
    err.outputFile = m_currentOutputFile;
    m_lastError = err;
    LOG_ERROR("ImageMagick", err.message);
    ErrorHandler::instance()->handleError(err);
    emit errorOccurred(err);
    emit statusChanged(tr("转换失败"));
    emit conversionFinished(false, err.message);
}
