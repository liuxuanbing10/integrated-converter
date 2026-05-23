#ifndef SKILL_MANAGER_H
#define SKILL_MANAGER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <QProcess>
#include <QTimer>
#include <memory>

struct SkillInfo {
    QString name;
    QString description;
    QString category;
    QVariantMap paramSchema;
    bool isBuiltIn;
};

struct SkillExecution {
    QString skillId;
    QString skillName;
    QProcess* process;
    QTimer* timeoutTimer;
    QVariantMap params;
    bool isRunning;
    QString outputBuffer;
    int progress;
};

class SkillManager : public QObject {
    Q_OBJECT

public:
    static SkillManager* instance();

    void invokeSkill(const QString& skillName, const QVariantMap& params = {});
    void cancelSkill(const QString& skillId);

    QStringList availableSkills() const;
    QStringList skillsByCategory(const QString& category) const;
    QStringList categories() const;
    SkillInfo skillInfo(const QString& skillName) const;
    QString skillDescription(const QString& skillName) const;
    bool isSkillRunning(const QString& skillId) const;

    void registerCustomSkill(const SkillInfo& skill);
    void unregisterCustomSkill(const QString& skillName);

signals:
    void skillStarted(const QString& skillId, const QString& skillName);
    void skillProgress(const QString& skillId, int progress, const QString& message);
    void skillFinished(const QString& skillId, bool success, const QVariant& result);
    void skillError(const QString& skillId, const QString& error);
    void skillOutput(const QString& skillId, const QString& output);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onProcessReadyReadStandardOutput();
    void onProcessReadyReadStandardError();
    void onTimeout();

private:
    SkillManager();
    ~SkillManager();
    SkillManager(const SkillManager&) = delete;
    SkillManager& operator=(const SkillManager&) = delete;

    void initBuiltInSkills();
    QString generateSkillId();
    void executeBuiltInSkill(const QString& skillId, const QString& skillName, const QVariantMap& params);
    void executeExternalSkill(const QString& skillId, const QString& skillName, const QVariantMap& params);
    void parseProgressOutput(const QString& skillId, const QString& output);
    void finishSkill(const QString& skillId, bool success, const QVariant& result);

    QMap<QString, SkillInfo> m_skills;
    QMap<QString, SkillExecution> m_runningExecutions;
    QMap<QString, QProcess*> m_processes;
    QMap<QString, QTimer*> m_timeoutTimers;
    int m_skillIdCounter;
    int m_defaultTimeout;
};

#endif // SKILL_MANAGER_H
