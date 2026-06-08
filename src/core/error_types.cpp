#include "error_types.h"
#include <algorithm>
#include <array>
#include <ranges>

namespace ErrorTypes {

QString errorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::Unknown: return QStringLiteral("Unknown");
        case ErrorCode::InvalidParameter: return QStringLiteral("InvalidParameter");
        case ErrorCode::FileNotFound: return QStringLiteral("FileNotFound");
        case ErrorCode::PermissionDenied: return QStringLiteral("PermissionDenied");
        case ErrorCode::DiskSpaceInsufficient: return QStringLiteral("DiskSpaceInsufficient");
        case ErrorCode::ConverterNotFound: return QStringLiteral("ConverterNotFound");
        case ErrorCode::ConverterNotAvailable: return QStringLiteral("ConverterNotAvailable");
        case ErrorCode::UnsupportedFormat: return QStringLiteral("UnsupportedFormat");
        case ErrorCode::ConversionFailed: return QStringLiteral("ConversionFailed");
        case ErrorCode::TaskCancelled: return QStringLiteral("TaskCancelled");
        case ErrorCode::TaskTimeout: return QStringLiteral("TaskTimeout");
        case ErrorCode::TaskDependencyFailed: return QStringLiteral("TaskDependencyFailed");
        case ErrorCode::ProcessCrashed: return QStringLiteral("ProcessCrashed");
        case ErrorCode::OutOfMemory: return QStringLiteral("OutOfMemory");
        case ErrorCode::ProcessFailedToStart: return QStringLiteral("ProcessFailedToStart");
        default: return QStringLiteral("Unknown");
    }
}

ErrorCode stringToErrorCode(const QString& str) {
    // C++23: constexpr lookup table using CTAD + ranges
    static constexpr std::array table = {
        std::pair{"ConversionFailed", ErrorCode::ConversionFailed},
        std::pair{"ConverterNotAvailable", ErrorCode::ConverterNotAvailable},
        std::pair{"ConverterNotFound", ErrorCode::ConverterNotFound},
        std::pair{"DiskSpaceInsufficient", ErrorCode::DiskSpaceInsufficient},
        std::pair{"FileNotFound", ErrorCode::FileNotFound},
        std::pair{"InvalidParameter", ErrorCode::InvalidParameter},
        std::pair{"OutOfMemory", ErrorCode::OutOfMemory},
        std::pair{"PermissionDenied", ErrorCode::PermissionDenied},
        std::pair{"ProcessCrashed", ErrorCode::ProcessCrashed},
        std::pair{"ProcessFailedToStart", ErrorCode::ProcessFailedToStart},
        std::pair{"TaskCancelled", ErrorCode::TaskCancelled},
        std::pair{"TaskDependencyFailed", ErrorCode::TaskDependencyFailed},
        std::pair{"TaskTimeout", ErrorCode::TaskTimeout},
        std::pair{"Unknown", ErrorCode::Unknown},
        std::pair{"UnsupportedFormat", ErrorCode::UnsupportedFormat},
    };

    auto it = std::ranges::find_if(table, [&str](const auto& pair) {
        return str == pair.first;
    });
    return it != table.end() ? it->second : ErrorCode::Unknown;
}

QString defaultSuggestion(ErrorCode code) {
    switch (code) {
        case ErrorCode::FileNotFound:
            return QObject::tr("请检查文件路径是否正确，确认文件是否存在");
        case ErrorCode::PermissionDenied:
            return QObject::tr("请尝试以管理员身份运行程序，或检查文件权限设置");
        case ErrorCode::DiskSpaceInsufficient:
            return QObject::tr("请清理磁盘空间或更换输出目录");
        case ErrorCode::ConverterNotFound:
            return QObject::tr("请检查转换器配置是否正确");
        case ErrorCode::ConverterNotAvailable:
            return QObject::tr("请下载并安装相应的转换工具（如FFmpeg或Pandoc），并确保已添加到系统PATH");
        case ErrorCode::UnsupportedFormat:
            return QObject::tr("请查看支持的格式列表，选择正确的输入输出格式");
        case ErrorCode::ConversionFailed:
            return QObject::tr("请检查输入文件是否损坏，或尝试不同的转换参数");
        case ErrorCode::TaskCancelled:
            return QObject::tr("任务已被取消，如需继续请重新添加任务");
        case ErrorCode::TaskTimeout:
            return QObject::tr("任务执行超时，可能是文件过大或系统资源不足");
        case ErrorCode::ProcessCrashed:
            return QObject::tr("转换进程异常退出，请检查系统环境和转换工具");
        case ErrorCode::OutOfMemory:
            return QObject::tr("内存不足，请尝试关闭其他程序或处理较小的文件");
        case ErrorCode::ProcessFailedToStart:
            return QObject::tr("无法启动转换进程，请检查转换工具是否正确安装");
        default:
            return QObject::tr("请检查操作是否正确，或查看日志获取详细信息");
    }
}

bool isRecoverable(ErrorCode code) {
    switch (code) {
        case ErrorCode::FileNotFound:
        case ErrorCode::PermissionDenied:
        case ErrorCode::DiskSpaceInsufficient:
        case ErrorCode::ConverterNotAvailable:
        case ErrorCode::ConversionFailed:
        case ErrorCode::TaskTimeout:
            return true;
        default:
            return false;
    }
}

ErrorInfo createError(ErrorCode code, const QString& message,
                     const QString& context, const QString& taskId) {
    ErrorInfo error;
    error.code = code;
    error.message = message;
    error.context = context;
    error.taskId = taskId;
    error.suggestion = defaultSuggestion(code);
    error.recoverable = isRecoverable(code);
    return error;
}

ErrorInfo createFileNotFoundError(const QString& filePath, const QString& context) {
    ErrorInfo error;
    error.code = ErrorCode::FileNotFound;
    error.message = QObject::tr("文件不存在");
    error.details = QObject::tr("文件路径: %1").arg(filePath);
    error.context = context;
    error.inputFile = filePath;
    error.suggestion = defaultSuggestion(ErrorCode::FileNotFound);
    error.recoverable = true;
    return error;
}

ErrorInfo createPermissionDeniedError(const QString& filePath, const QString& context) {
    ErrorInfo error;
    error.code = ErrorCode::PermissionDenied;
    error.message = QObject::tr("权限不足");
    error.details = QObject::tr("文件路径: %1").arg(filePath);
    error.context = context;
    error.inputFile = filePath;
    error.suggestion = defaultSuggestion(ErrorCode::PermissionDenied);
    error.recoverable = true;
    return error;
}

ErrorInfo createConverterNotAvailableError(const QString& converterName, const QString& context) {
    ErrorInfo error;
    error.code = ErrorCode::ConverterNotAvailable;
    error.message = QObject::tr("转换器不可用");
    error.details = QObject::tr("转换器: %1").arg(converterName);
    error.context = context;
    error.converterName = converterName;
    error.suggestion = defaultSuggestion(ErrorCode::ConverterNotAvailable);
    error.recoverable = true;
    return error;
}

ErrorInfo createUnsupportedFormatError(const QString& inputFormat,
                                      const QString& outputFormat,
                                      const QString& context) {
    ErrorInfo error;
    error.code = ErrorCode::UnsupportedFormat;
    error.message = QObject::tr("不支持的格式转换");
    error.details = QObject::tr("输入格式: %1, 输出格式: %2").arg(inputFormat, outputFormat);
    error.context = context;
    error.suggestion = defaultSuggestion(ErrorCode::UnsupportedFormat);
    error.recoverable = false;
    return error;
}

ErrorInfo createConversionFailedError(const QString& details,
                                     const QString& converterName,
                                     const QString& context) {
    ErrorInfo error;
    error.code = ErrorCode::ConversionFailed;
    error.message = QObject::tr("转换失败");
    error.details = details;
    error.context = context;
    error.converterName = converterName;
    error.suggestion = defaultSuggestion(ErrorCode::ConversionFailed);
    error.recoverable = true;
    return error;
}

ErrorInfo createTaskCancelledError(const QString& taskId, const QString& context) {
    ErrorInfo error;
    error.code = ErrorCode::TaskCancelled;
    error.message = QObject::tr("任务已取消");
    error.context = context;
    error.taskId = taskId;
    error.suggestion = defaultSuggestion(ErrorCode::TaskCancelled);
    error.recoverable = false;
    return error;
}

ErrorInfo createProcessError(ErrorCode code, const QString& processName,
                            const QString& details, const QString& context) {
    ErrorInfo error;
    error.code = code;
    error.converterName = processName;
    error.context = context;
    switch (code) {
        case ErrorCode::ProcessCrashed:
            error.message = QObject::tr("进程崩溃");
            break;
        case ErrorCode::ProcessFailedToStart:
            error.message = QObject::tr("进程启动失败");
            break;
        case ErrorCode::TaskTimeout:
            error.message = QObject::tr("进程执行超时");
            break;
        default:
            error.message = QObject::tr("进程错误");
            break;
    }
    if (!processName.isEmpty()) {
        error.details = QObject::tr("进程: %1").arg(processName);
        if (!details.isEmpty()) {
            error.details += "\n" + details;
        }
    } else {
        error.details = details;
    }
    error.suggestion = defaultSuggestion(code);
    error.recoverable = isRecoverable(code);
    return error;
}

}
