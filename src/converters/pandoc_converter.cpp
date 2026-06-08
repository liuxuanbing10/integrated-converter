#include "pandoc_converter.h"
#include "config_manager.h"
#include "format_registry.h"
#include "logger.h"
#include "error_handler.h"
#include <QFileInfo>
#include <QDir>
PandocConverter::PandocConverter(QObject* parent)
    : QObject(parent)
    , m_pandocPath("pandoc")
    , m_currentProcess(nullptr)
    , m_isConverting(false)
{
    m_pandocPath = ConfigManager::instance().value("pandocPath", "pandoc").toString();
    const auto& reg = FormatRegistry::instance();
    m_inputFormats = reg.documentInputFormats();
    m_outputFormats = reg.documentOutputFormats();
}
PandocConverter::~PandocConverter() {
    if (m_currentProcess) {
        m_currentProcess->kill();
        m_currentProcess.reset();
    }
}
QString PandocConverter::getPandocFormat(const QString& format) const {
    return FormatRegistry::instance().pandocFormatName(format);
}
bool PandocConverter::checkPandocAvailable() const {
    QProcess process;
    process.setProgram(m_pandocPath);
    process.setArguments(QStringList() << "--version");
    process.start();
    return process.waitForStarted() && process.waitForFinished(5000) && process.exitCode() == 0;
}
QString PandocConverter::getPandocVersion() const {
    QProcess process;
    process.setProgram(m_pandocPath);
    process.setArguments(QStringList() << "--version");
    process.start();
    if (process.waitForStarted() && process.waitForFinished(5000)) {
        QString output = process.readAllStandardOutput();
        QStringList lines = output.split('\n');
        if (!lines.isEmpty()) {
            return lines.first();
        }
    }
    return QString();
}
QString PandocConverter::detectInputFormat(const QString& filePath) const {
    QString ext = QFileInfo(filePath).suffix().toLower();
    return getPandocFormat(ext);
}
QString PandocConverter::detectOutputFormat(const QString& filePath) const {
    QString ext = QFileInfo(filePath).suffix().toLower();
    return getPandocFormat(ext);
}
QStringList PandocConverter::buildArguments(const QString& inputFile,
                                           const QString& outputFile,
                                           const QVariantMap& params) {
    QStringList args;
    QString fromFormat = params.value("from").toString();
    if (fromFormat.isEmpty()) {
        fromFormat = detectInputFormat(inputFile);
    }
    if (!fromFormat.isEmpty()) {
        args << "--from" << getPandocFormat(fromFormat);
    }
    QString toFormat = params.value("to").toString();
    if (toFormat.isEmpty()) {
        toFormat = detectOutputFormat(outputFile);
    }
    if (!toFormat.isEmpty()) {
        QString pandocToFormat = getPandocFormat(toFormat);
        if (toFormat.toLower() == "pdf") {
            args << "--to" << "pdf";
        } else {
            args << "--to" << pandocToFormat;
        }
    }
    args << inputFile;
    args << "-o" << outputFile;
    bool standalone = params.value("standalone", true).toBool();
    if (standalone) {
        args << "--standalone";
    }
    QString templatePath = params.value("template").toString();
    if (!templatePath.isEmpty() && QFileInfo(templatePath).exists()) {
        args << "--template" << templatePath;
    }
    QString referenceDoc = params.value("referenceDoc").toString();
    if (!referenceDoc.isEmpty() && QFileInfo(referenceDoc).exists()) {
        args << "--reference-doc" << referenceDoc;
    }
    QString cssPath = params.value("css").toString();
    if (!cssPath.isEmpty()) {
        args << "--css" << cssPath;
    }
    bool toc = params.value("toc", false).toBool();
    if (toc) {
        args << "--toc";
        int tocDepth = params.value("tocDepth", 3).toInt();
        if (tocDepth > 0 && tocDepth <= 6) {
            args << "--toc-depth" << QString::number(tocDepth);
        }
    }
    QString pdfEngine = params.value("pdfEngine").toString();
    QString outputFormatLower = toFormat.toLower();
    if (outputFormatLower == "pdf") {
        if (pdfEngine.isEmpty()) {
            pdfEngine = "xelatex";
        }
        args << "--pdf-engine" << pdfEngine;
        QVariantMap pdfEngineOpts = params.value("pdfEngineOpts").toMap();
        if (!pdfEngineOpts.isEmpty()) {
            for (auto it = pdfEngineOpts.begin(); it != pdfEngineOpts.end(); ++it) {
                args << "--pdf-engine-opt" << QString("%1=%2").arg(it.key(), it.value().toString());
            }
        }
    }
    QString highlightStyle = params.value("highlightStyle").toString();
    if (!highlightStyle.isEmpty()) {
        args << "--highlight-style" << highlightStyle;
    }
    QStringList variables = params.value("variables").toStringList();
    for (const QString& var : variables) {
        args << "-V" << var;
    }
    QVariantMap variableMap = params.value("variableMap").toMap();
    for (auto it = variableMap.begin(); it != variableMap.end(); ++it) {
        args << "-V" << QString("%1=%2").arg(it.key(), it.value().toString());
    }
    if (outputFormatLower == "pdf" || toFormat.toLower() == "latex") {
        if (!variableMap.contains("geometry:margin") && !variables.contains("geometry:margin")) {
            args << "-V" << "geometry:margin=1in";
        }
    }
    QString metadataFile = params.value("metadataFile").toString();
    if (!metadataFile.isEmpty() && QFileInfo(metadataFile).exists()) {
        args << "--metadata-file" << metadataFile;
    }
    QStringList metadata = params.value("metadata").toStringList();
    for (const QString& meta : metadata) {
        args << "--metadata" << meta;
    }
    QString resourcePath = params.value("resourcePath").toString();
    if (!resourcePath.isEmpty()) {
        args << "--resource-path" << resourcePath;
    }
    QString dataDir = params.value("dataDir").toString();
    if (!dataDir.isEmpty()) {
        args << "--data-dir" << dataDir;
    }
    bool extractMedia = params.value("extractMedia", false).toBool();
    if (extractMedia) {
        QString mediaDir = params.value("mediaDir").toString();
        if (mediaDir.isEmpty()) {
            QFileInfo outputInfo(outputFile);
            mediaDir = outputInfo.absolutePath() + "/media";
        }
        args << "--extract-media" << mediaDir;
    }
    QString wrap = params.value("wrap").toString();
    if (!wrap.isEmpty()) {
        args << "--wrap" << wrap;
    }
    int columns = params.value("columns", 0).toInt();
    if (columns > 0) {
        args << "--columns" << QString::number(columns);
    }
    bool preserveTabs = params.value("preserveTabs", false).toBool();
    if (preserveTabs) {
        args << "--preserve-tabs";
    }
    bool shiftHeadingLevel = params.value("shiftHeadingLevel", false).toBool();
    if (shiftHeadingLevel) {
        int shift = params.value("shiftHeading", 0).toInt();
        args << "--shift-heading-level-by" << QString::number(shift);
    }
    QString includeBeforeBody = params.value("includeBeforeBody").toString();
    if (!includeBeforeBody.isEmpty() && QFileInfo(includeBeforeBody).exists()) {
        args << "--include-before-body" << includeBeforeBody;
    }
    QString includeAfterBody = params.value("includeAfterBody").toString();
    if (!includeAfterBody.isEmpty() && QFileInfo(includeAfterBody).exists()) {
        args << "--include-after-body" << includeAfterBody;
    }
    QString includeInHeader = params.value("includeInHeader").toString();
    if (!includeInHeader.isEmpty() && QFileInfo(includeInHeader).exists()) {
        args << "--include-in-header" << includeInHeader;
    }
    QStringList extraArgs = params.value("extraArgs").toStringList();
    args << extraArgs;
    return args;
}
std::optional<ErrorInfo> PandocConverter::convert(const QString& inputFile, const QString& outputFile,
                                                   const QVariantMap& params) {
    LOG_INFO("Pandoc", QString("开始转换: %1 -> %2").arg(inputFile, outputFile));
    if (m_isConverting) {
        LOG_WARNING("Pandoc", "已有转换任务正在执行");
        emit conversionFinished(false, tr("已有转换任务正在执行"));
        return ErrorTypes::createError(ErrorCode::ConverterNotAvailable,
                                       tr("已有转换任务正在执行"), "Pandoc::convert");
    }
    QFileInfo inputInfo(inputFile);
    if (!inputInfo.exists()) {
        ErrorInfo error = ErrorTypes::createFileNotFoundError(inputFile, "Pandoc::convert");
        error.outputFile = outputFile;
        m_lastError = error;
        LOG_ERROR("Pandoc", QString("输入文件不存在: %1").arg(inputFile));
        ErrorHandler::instance()->handleError(error);
        emit errorOccurred(error);
        emit conversionFinished(false, error.message);
        return error;
    }
    m_currentInputFile = inputFile;
    m_currentOutputFile = outputFile;
    QFileInfo outputInfo(outputFile);
    QDir outputDir = outputInfo.absoluteDir();
    if (!outputDir.exists()) {
        if (!outputDir.mkpath(".")) {
            ErrorInfo error = ErrorTypes::createError(
                ErrorCode::PermissionDenied,
                tr("无法创建输出目录"),
                "Pandoc::convert"
            );
            error.inputFile = inputFile;
            error.outputFile = outputFile;
            m_lastError = error;
            LOG_ERROR("Pandoc", QString("无法创建输出目录: %1").arg(outputDir.absolutePath()));
            ErrorHandler::instance()->handleError(error);
            emit errorOccurred(error);
            emit conversionFinished(false, error.message);
            return error;
        }
    }
    QStringList args = buildArguments(inputFile, outputFile, params);
    QString output;
    bool success = runPandoc(args, output);
    if (success) {
        LOG_INFO("Pandoc", QString("转换完成: %1").arg(outputFile));
        emit conversionFinished(true, tr("转换成功"));
        return std::nullopt;
    }
    ErrorInfo error = ErrorTypes::createConversionFailedError(
        output, "Pandoc", "Pandoc::convert"
    );
    error.inputFile = inputFile;
    error.outputFile = outputFile;
    m_lastError = error;
    LOG_ERROR("Pandoc", QString("转换失败: %1").arg(output));
    ErrorHandler::instance()->handleError(error);
    emit errorOccurred(error);
    emit conversionFinished(false, error.message);
    return error;
}
bool PandocConverter::runPandoc(const QStringList& args, QString& output) {
    QProcess process;
    process.setProgram(m_pandocPath);
    process.setArguments(args);
    LOG_DEBUG("Pandoc", QString("执行命令: %1 %2").arg(m_pandocPath, args.join(" ")));
    process.start();
    if (!process.waitForStarted()) {
        output = tr("无法启动Pandoc进程: %1").arg(process.errorString());
        return false;
    }
    emit statusChanged(tr("正在转换..."));
    emit progressChanged(50);
    if (!process.waitForFinished(-1)) {
        output = tr("Pandoc进程执行超时");
        return false;
    }
    output = process.readAllStandardError();
    if (output.isEmpty()) {
        output = process.readAllStandardOutput();
    }
    emit progressChanged(100);
    return process.exitCode() == 0;
}
bool PandocConverter::runPandocAsync(const QString& inputFile,
                                    const QString& outputFile,
                                    const QStringList& args) {
    if (m_currentProcess) {
        m_currentProcess->kill();
        m_currentProcess.reset();
    }
    m_currentProcess = std::make_unique<QProcess>();
    m_currentOutputFile = outputFile;
    m_isConverting = true;
    connect(m_currentProcess.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &PandocConverter::onProcessFinished);
    connect(m_currentProcess.get(), &QProcess::errorOccurred,
            this, &PandocConverter::onProcessError);
    connect(m_currentProcess.get(), &QProcess::readyReadStandardError,
            this, &PandocConverter::onProcessReadyReadStandardError);
    m_currentProcess->setProgram(m_pandocPath);
    m_currentProcess->setArguments(args);
    LOG_DEBUG("Pandoc", QString("异步执行命令: %1 %2").arg(m_pandocPath, args.join(" ")));
    m_currentProcess->start();
    return m_currentProcess->waitForStarted();
}
void PandocConverter::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    QString output;
    if (m_currentProcess) {
        output = m_currentProcess->readAllStandardError();
        if (output.isEmpty()) {
            output = m_currentProcess->readAllStandardOutput();
        }
    }
    m_isConverting = false;
    m_currentProcess.reset();
    bool success = (exitCode == 0 && exitStatus == QProcess::NormalExit);
    if (success) {
        LOG_INFO("Pandoc", QString("异步转换完成: %1").arg(m_currentOutputFile));
        emit progressChanged(100);
        emit statusChanged(tr("转换完成"));
        emit conversionFinished(true, tr("转换成功"));
    } else {
        LOG_ERROR("Pandoc", QString("异步转换失败: %1").arg(output));
        emit conversionFinished(false, output);
    }
}
void PandocConverter::onProcessError(QProcess::ProcessError error) {
    m_isConverting = false;
    m_currentProcess.reset();
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
    ErrorInfo err = ErrorTypes::createProcessError(errorCode, "Pandoc",
                                                   QString(), "Pandoc::onProcessError");
    err.inputFile = m_currentInputFile;
    err.outputFile = m_currentOutputFile;
    m_lastError = err;
    LOG_ERROR("Pandoc", QString("进程错误: %1").arg(err.message));
    ErrorHandler::instance()->handleError(err);
    emit errorOccurred(err);
    emit conversionFinished(false, err.message);
}
void PandocConverter::onProcessReadyReadStandardError() {
    if (m_currentProcess) {
        QString output = m_currentProcess->readAllStandardError();
        LOG_DEBUG("Pandoc", output);
        emit statusChanged(output.trimmed());
    }
}
QStringList PandocConverter::supportedInputFormats() const {
    return m_inputFormats;
}
QStringList PandocConverter::supportedOutputFormats() const {
    return m_outputFormats;
}
bool PandocConverter::isConversionSupported(const QString& inputFormat,
                                          const QString& outputFormat) const {
    QString input = inputFormat.toLower();
    QString output = outputFormat.toLower();
    bool inputSupported = m_inputFormats.contains(input);
    bool outputSupported = m_outputFormats.contains(output);
    return inputSupported && outputSupported;
}
