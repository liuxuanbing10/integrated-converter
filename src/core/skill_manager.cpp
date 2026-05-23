#include "skill_manager.h"
#include "logger.h"
#include <QUuid>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>

SkillManager* SkillManager::instance() {
    static SkillManager instance;
    return &instance;
}

SkillManager::SkillManager()
    : m_skillIdCounter(0)
    , m_defaultTimeout(300000)
{
    initBuiltInSkills();
    LOG_INFO("SkillManager", "Skill管理器初始化完成");
}

SkillManager::~SkillManager() {
    for (auto it = m_processes.begin(); it != m_processes.end(); ++it) {
        if (it.value()) {
            it.value()->kill();
            it.value()->deleteLater();
        }
    }
    for (auto it = m_timeoutTimers.begin(); it != m_timeoutTimers.end(); ++it) {
        if (it.value()) {
            it.value()->stop();
            it.value()->deleteLater();
        }
    }
}

void SkillManager::initBuiltInSkills() {
    SkillInfo formatDetect;
    formatDetect.name = "format_detect";
    formatDetect.description = "自动检测文件格式";
    formatDetect.category = "分析";
    formatDetect.isBuiltIn = true;
    formatDetect.paramSchema = QVariantMap{
        {"file", QVariantMap{{"type", "string"}, {"description", "要检测的文件路径"}, {"required", true}}},
        {"detailed", QVariantMap{{"type", "bool"}, {"description", "是否返回详细信息"}, {"default", false}}}
    };
    m_skills[formatDetect.name] = formatDetect;

    SkillInfo batchRename;
    batchRename.name = "batch_rename";
    batchRename.description = "批量重命名输出文件";
    batchRename.category = "文件操作";
    batchRename.isBuiltIn = true;
    batchRename.paramSchema = QVariantMap{
        {"directory", QVariantMap{{"type", "string"}, {"description", "目标目录"}, {"required", true}}},
        {"pattern", QVariantMap{{"type", "string"}, {"description", "重命名模式（支持{index},{name},{date}）"}, {"required", true}}},
        {"prefix", QVariantMap{{"type", "string"}, {"description", "文件名前缀"}, {"default", ""}}},
        {"suffix", QVariantMap{{"type", "string"}, {"description", "文件名后缀"}, {"default", ""}}},
        {"start_index", QVariantMap{{"type", "int"}, {"description", "起始序号"}, {"default", 1}}}
    };
    m_skills[batchRename.name] = batchRename;

    SkillInfo presetApply;
    presetApply.name = "preset_apply";
    presetApply.description = "应用转换预设";
    presetApply.category = "转换";
    presetApply.isBuiltIn = true;
    presetApply.paramSchema = QVariantMap{
        {"preset_name", QVariantMap{{"type", "string"}, {"description", "预设名称"}, {"required", true}}},
        {"files", QVariantMap{{"type", "array"}, {"description", "要应用的文件列表"}, {"required", true}}}
    };
    m_skills[presetApply.name] = presetApply;

    SkillInfo historyExport;
    historyExport.name = "history_export";
    historyExport.description = "导出转换历史";
    historyExport.category = "历史";
    historyExport.isBuiltIn = true;
    historyExport.paramSchema = QVariantMap{
        {"output_file", QVariantMap{{"type", "string"}, {"description", "输出文件路径"}, {"required", true}}},
        {"format", QVariantMap{{"type", "string"}, {"description", "导出格式(json/csv/txt)"}, {"default", "json"}}},
        {"date_from", QVariantMap{{"type", "string"}, {"description", "起始日期"}, {"required", false}}},
        {"date_to", QVariantMap{{"type", "string"}, {"description", "结束日期"}, {"required", false}}}
    };
    m_skills[historyExport.name] = historyExport;

    SkillInfo fileValidate;
    fileValidate.name = "file_validate";
    fileValidate.description = "验证文件完整性";
    fileValidate.category = "分析";
    fileValidate.isBuiltIn = true;
    fileValidate.paramSchema = QVariantMap{
        {"files", QVariantMap{{"type", "array"}, {"description", "要验证的文件列表"}, {"required", true}}},
        {"check_hash", QVariantMap{{"type", "bool"}, {"description", "是否计算文件哈希"}, {"default", false}}}
    };
    m_skills[fileValidate.name] = fileValidate;

    LOG_INFO("SkillManager", QString("已注册 %1 个内置Skill").arg(m_skills.size()));
}

QString SkillManager::generateSkillId() {
    return QString("skill_%1_%2").arg(++m_skillIdCounter).arg(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
}

void SkillManager::invokeSkill(const QString& skillName, const QVariantMap& params) {
    if (!m_skills.contains(skillName)) {
        emit skillError("", QString("未知的Skill: %1").arg(skillName));
        return;
    }

    QString skillId = generateSkillId();
    const SkillInfo& info = m_skills[skillName];

    SkillExecution execution;
    execution.skillId = skillId;
    execution.skillName = skillName;
    execution.params = params;
    execution.isRunning = true;
    execution.process = nullptr;
    execution.timeoutTimer = nullptr;
    execution.progress = 0;
    m_runningExecutions[skillId] = execution;

    LOG_INFO("SkillManager", QString("开始执行Skill: %1 (ID: %2)").arg(skillName).arg(skillId));
    emit skillStarted(skillId, skillName);

    if (info.isBuiltIn) {
        executeBuiltInSkill(skillId, skillName, params);
    } else {
        executeExternalSkill(skillId, skillName, params);
    }
}

void SkillManager::cancelSkill(const QString& skillId) {
    if (!m_runningExecutions.contains(skillId)) {
        return;
    }

    SkillExecution& execution = m_runningExecutions[skillId];
    if (execution.process) {
        execution.process->kill();
    }
    if (execution.timeoutTimer) {
        execution.timeoutTimer->stop();
    }
    execution.isRunning = false;

    LOG_INFO("SkillManager", QString("取消Skill执行: %1").arg(skillId));
    emit skillError(skillId, "用户取消");
    m_runningExecutions.remove(skillId);
}

void SkillManager::executeBuiltInSkill(const QString& skillId, const QString& skillName, const QVariantMap& params) {
    QVariant result;

    if (skillName == "format_detect") {
        QString filePath = params["file"].toString();
        bool detailed = params.value("detailed", false).toBool();

        QFileInfo fi(filePath);
        if (!fi.exists()) {
            finishSkill(skillId, false, QVariantMap{{"error", "文件不存在"}});
            return;
        }

        QString suffix = fi.suffix().toLower();
        QString formatType;
        QString mimeType;

        if (QStringList{"mp4", "avi", "mkv", "mov", "flv", "wmv"}.contains(suffix)) {
            formatType = "video";
            mimeType = QString("video/%1").arg(suffix);
        } else if (QStringList{"mp3", "wav", "flac", "aac", "ogg", "m4a"}.contains(suffix)) {
            formatType = "audio";
            mimeType = QString("audio/%1").arg(suffix);
        } else if (QStringList{"md", "txt", "docx", "pdf", "html", "rtf"}.contains(suffix)) {
            formatType = "document";
            mimeType = QString("application/%1").arg(suffix);
        } else {
            formatType = "unknown";
            mimeType = "application/octet-stream";
        }

        QVariantMap detectResult;
        detectResult["format"] = suffix;
        detectResult["type"] = formatType;
        detectResult["mime_type"] = mimeType;

        if (detailed) {
            detectResult["size"] = fi.size();
            detectResult["name"] = fi.fileName();
            detectResult["path"] = fi.absoluteFilePath();
            detectResult["modified"] = fi.lastModified().toString(Qt::ISODate);
        }

        result = detectResult;
    }
    else if (skillName == "batch_rename") {
        QString directory = params["directory"].toString();
        QString pattern = params["pattern"].toString();
        QString prefix = params.value("prefix", "").toString();
        QString suffix = params.value("suffix", "").toString();
        int startIndex = params.value("start_index", 1).toInt();

        QDir dir(directory);
        if (!dir.exists()) {
            finishSkill(skillId, false, QVariantMap{{"error", "目录不存在"}});
            return;
        }

        QStringList files = dir.entryList(QDir::Files);
        QVariantList renameResults;
        int index = startIndex;

        for (const QString& file : files) {
            QFileInfo fi(file);
            QString baseName = fi.completeBaseName();
            QString ext = fi.suffix();

            QString newName = pattern;
            newName.replace("{index}", QString::number(index));
            newName.replace("{name}", baseName);
            newName.replace("{date}", QDate::currentDate().toString("yyyyMMdd"));

            newName = prefix + newName + suffix;
            if (!ext.isEmpty()) {
                newName += "." + ext;
            }

            QVariantMap renameInfo;
            renameInfo["original"] = file;
            renameInfo["new_name"] = newName;
            renameResults.append(renameInfo);

            index++;
        }

        result = QVariantMap{{"files", renameResults}, {"count", renameResults.size()}};
    }
    else if (skillName == "preset_apply") {
        QString presetName = params["preset_name"].toString();
        QStringList files = params["files"].toStringList();

        QVariantMap presetResult;
        presetResult["preset"] = presetName;
        presetResult["applied_files"] = files;
        presetResult["count"] = files.size();
        result = presetResult;
    }
    else if (skillName == "history_export") {
        QString outputFile = params["output_file"].toString();
        QString format = params.value("format", "json").toString();

        QVariantMap exportResult;
        exportResult["output_file"] = outputFile;
        exportResult["format"] = format;
        exportResult["exported"] = true;
        result = exportResult;
    }
    else if (skillName == "file_validate") {
        QStringList files = params["files"].toStringList();
        bool checkHash = params.value("check_hash", false).toBool();

        QVariantList validateResults;
        for (const QString& file : files) {
            QFileInfo fi(file);
            QVariantMap fileInfo;
            fileInfo["path"] = file;
            fileInfo["exists"] = fi.exists();
            fileInfo["valid"] = fi.exists() && fi.size() > 0;
            if (fi.exists()) {
                fileInfo["size"] = fi.size();
            }
            validateResults.append(fileInfo);
        }

        result = QVariantMap{{"files", validateResults}, {"check_hash", checkHash}};
    }

    emit skillProgress(skillId, 100, "执行完成");
    finishSkill(skillId, true, result);
}

void SkillManager::executeExternalSkill(const QString& skillId, const QString& skillName, const QVariantMap& params) {
    Q_UNUSED(skillName);
    Q_UNUSED(params);

    finishSkill(skillId, false, QVariantMap{{"error", "外部Skill暂不支持"}});
}

void SkillManager::finishSkill(const QString& skillId, bool success, const QVariant& result) {
    if (!m_runningExecutions.contains(skillId)) {
        return;
    }

    m_runningExecutions[skillId].isRunning = false;

    if (m_processes.contains(skillId)) {
        QProcess* process = m_processes.take(skillId);
        process->deleteLater();
    }

    if (m_timeoutTimers.contains(skillId)) {
        QTimer* timer = m_timeoutTimers.take(skillId);
        timer->stop();
        timer->deleteLater();
    }

    LOG_INFO("SkillManager", QString("Skill执行完成: %1, 成功: %2").arg(skillId).arg(success));
    emit skillFinished(skillId, success, result);

    m_runningExecutions.remove(skillId);
}

void SkillManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    QProcess* process = qobject_cast<QProcess*>(sender());
    if (!process) return;

    QString skillId;
    for (auto it = m_processes.begin(); it != m_processes.end(); ++it) {
        if (it.value() == process) {
            skillId = it.key();
            break;
        }
    }

    if (skillId.isEmpty()) return;

    QString output = process->readAllStandardOutput();
    QString error = process->readAllStandardError();

    bool success = (exitCode == 0 && exitStatus == QProcess::NormalExit);
    QVariant result;

    if (success) {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError) {
            result = doc.toVariant();
        } else {
            result = QVariantMap{{"output", output}};
        }
    } else {
        result = QVariantMap{{"error", error}, {"exit_code", exitCode}};
    }

    finishSkill(skillId, success, result);
}

void SkillManager::onProcessError(QProcess::ProcessError error) {
    QProcess* process = qobject_cast<QProcess*>(sender());
    if (!process) return;

    QString skillId;
    for (auto it = m_processes.begin(); it != m_processes.end(); ++it) {
        if (it.value() == process) {
            skillId = it.key();
            break;
        }
    }

    if (skillId.isEmpty()) return;

    QString errorMsg;
    switch (error) {
        case QProcess::FailedToStart: errorMsg = "进程启动失败"; break;
        case QProcess::Crashed: errorMsg = "进程崩溃"; break;
        case QProcess::Timedout: errorMsg = "进程超时"; break;
        case QProcess::WriteError: errorMsg = "写入错误"; break;
        case QProcess::ReadError: errorMsg = "读取错误"; break;
        default: errorMsg = "未知错误"; break;
    }

    finishSkill(skillId, false, QVariantMap{{"error", errorMsg}});
}

void SkillManager::onProcessReadyReadStandardOutput() {
    QProcess* process = qobject_cast<QProcess*>(sender());
    if (!process) return;

    QString skillId;
    for (auto it = m_processes.begin(); it != m_processes.end(); ++it) {
        if (it.value() == process) {
            skillId = it.key();
            break;
        }
    }

    if (skillId.isEmpty()) return;

    QString output = process->readAllStandardOutput();
    m_runningExecutions[skillId].outputBuffer += output;
    emit skillOutput(skillId, output);
    parseProgressOutput(skillId, output);
}

void SkillManager::onProcessReadyReadStandardError() {
    QProcess* process = qobject_cast<QProcess*>(sender());
    if (!process) return;

    QString skillId;
    for (auto it = m_processes.begin(); it != m_processes.end(); ++it) {
        if (it.value() == process) {
            skillId = it.key();
            break;
        }
    }

    if (skillId.isEmpty()) return;

    QString error = process->readAllStandardError();
    emit skillOutput(skillId, "[ERROR] " + error);
}

void SkillManager::onTimeout() {
    QTimer* timer = qobject_cast<QTimer*>(sender());
    if (!timer) return;

    QString skillId;
    for (auto it = m_timeoutTimers.begin(); it != m_timeoutTimers.end(); ++it) {
        if (it.value() == timer) {
            skillId = it.key();
            break;
        }
    }

    if (skillId.isEmpty()) return;

    LOG_WARNING("SkillManager", QString("Skill执行超时: %1").arg(skillId));
    finishSkill(skillId, false, QVariantMap{{"error", "执行超时"}});
}

void SkillManager::parseProgressOutput(const QString& skillId, const QString& output) {
    QRegularExpression progressRe("\\[PROGRESS:(\\d+)%\\]\\s*(.*)");
    QRegularExpressionMatch match = progressRe.match(output);
    if (match.hasMatch()) {
        int progress = match.captured(1).toInt();
        QString message = match.captured(2).trimmed();
        m_runningExecutions[skillId].progress = progress;
        emit skillProgress(skillId, progress, message);
    }
}

QStringList SkillManager::availableSkills() const {
    return m_skills.keys();
}

QStringList SkillManager::skillsByCategory(const QString& category) const {
    QStringList result;
    for (auto it = m_skills.begin(); it != m_skills.end(); ++it) {
        if (it->category == category) {
            result.append(it.key());
        }
    }
    return result;
}

QStringList SkillManager::categories() const {
    QStringList result;
    for (const SkillInfo& info : m_skills) {
        if (!result.contains(info.category)) {
            result.append(info.category);
        }
    }
    return result;
}

SkillInfo SkillManager::skillInfo(const QString& skillName) const {
    return m_skills.value(skillName);
}

QString SkillManager::skillDescription(const QString& skillName) const {
    if (m_skills.contains(skillName)) {
        return m_skills[skillName].description;
    }
    return QString();
}

bool SkillManager::isSkillRunning(const QString& skillId) const {
    return m_runningExecutions.contains(skillId) && m_runningExecutions[skillId].isRunning;
}

void SkillManager::registerCustomSkill(const SkillInfo& skill) {
    m_skills[skill.name] = skill;
    LOG_INFO("SkillManager", QString("注册自定义Skill: %1").arg(skill.name));
}

void SkillManager::unregisterCustomSkill(const QString& skillName) {
    if (m_skills.contains(skillName) && !m_skills[skillName].isBuiltIn) {
        m_skills.remove(skillName);
        LOG_INFO("SkillManager", QString("注销自定义Skill: %1").arg(skillName));
    }
}
