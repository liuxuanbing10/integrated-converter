#ifndef SKILL_INVOKE_DIALOG_H
#define SKILL_INVOKE_DIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QScrollArea>
#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QProgressBar>
#include <QVariantMap>
#include <QList>

struct ParamWidget {
    QString name;
    QWidget* widget;
    QString type;
};

class SkillInvokeDialog : public QDialog {
    Q_OBJECT

public:
    explicit SkillInvokeDialog(QWidget* parent = nullptr);
    ~SkillInvokeDialog() override;

    void setPreselectedFiles(const QStringList& files);

signals:
    void skillResultReady(const QString& skillName, const QVariant& result);

private slots:
    void onSkillSelected(int index);
    void onExecuteClicked();
    void onCancelClicked();
    void onCopyResultClicked();
    void onExportResultClicked();
    void onApplyResultClicked();
    void onSkillStarted(const QString& skillId, const QString& skillName);
    void onSkillProgress(const QString& skillId, int progress, const QString& message);
    void onSkillFinished(const QString& skillId, bool success, const QVariant& result);
    void onSkillError(const QString& skillId, const QString& error);

private:
    void setupUI();
    void setupConnections();
    void populateSkillList();
    void updateParamWidgets();
    void clearParamWidgets();
    QVariantMap collectParams();
    void displayResult(const QVariant& result);
    QString formatResult(const QVariant& result, int indent = 0);
    void setExecuting(bool executing);

    QComboBox* m_skillComboBox;
    QLabel* m_descriptionLabel;
    QLabel* m_categoryLabel;
    QScrollArea* m_paramScrollArea;
    QWidget* m_paramContainer;
    QList<ParamWidget> m_paramWidgets;
    QPushButton* m_executeButton;
    QPushButton* m_cancelButton;
    QProgressBar* m_progressBar;
    QLabel* m_progressLabel;
    QTextEdit* m_resultTextEdit;
    QPushButton* m_copyResultButton;
    QPushButton* m_exportResultButton;
    QPushButton* m_applyResultButton;
    QLabel* m_statusLabel;

    QString m_currentSkillId;
    QString m_currentSkillName;
    QVariant m_lastResult;
    QStringList m_preselectedFiles;
    bool m_isExecuting;
};

#endif // SKILL_INVOKE_DIALOG_H
