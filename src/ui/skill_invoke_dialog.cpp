#include "skill_invoke_dialog.h"
#include "skill_manager.h"
#include "logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDateTime>

SkillInvokeDialog::SkillInvokeDialog(QWidget* parent)
    : QDialog(parent)
    , m_skillComboBox(nullptr)
    , m_descriptionLabel(nullptr)
    , m_categoryLabel(nullptr)
    , m_paramScrollArea(nullptr)
    , m_paramContainer(nullptr)
    , m_executeButton(nullptr)
    , m_cancelButton(nullptr)
    , m_progressBar(nullptr)
    , m_progressLabel(nullptr)
    , m_resultTextEdit(nullptr)
    , m_copyResultButton(nullptr)
    , m_exportResultButton(nullptr)
    , m_applyResultButton(nullptr)
    , m_statusLabel(nullptr)
    , m_isExecuting(false)
{
    setupUI();
    setupConnections();
    populateSkillList();
    setWindowTitle(tr("调用 Skill"));
    resize(600, 500);
    LOG_INFO("SkillInvokeDialog", "Skill调用对话框初始化完成");
}

SkillInvokeDialog::~SkillInvokeDialog() {
}

void SkillInvokeDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    QGroupBox* selectGroup = new QGroupBox(tr("选择 Skill"));
    QVBoxLayout* selectLayout = new QVBoxLayout(selectGroup);

    QHBoxLayout* skillRow = new QHBoxLayout();
    QLabel* skillLabel = new QLabel(tr("Skill:"));
    m_skillComboBox = new QComboBox();
    skillRow->addWidget(skillLabel);
    skillRow->addWidget(m_skillComboBox, 1);
    selectLayout->addLayout(skillRow);

    QHBoxLayout* infoRow = new QHBoxLayout();
    m_categoryLabel = new QLabel();
    m_categoryLabel->setStyleSheet("color: #666;");
    m_descriptionLabel = new QLabel();
    m_descriptionLabel->setWordWrap(true);
    m_descriptionLabel->setStyleSheet("color: #333;");
    infoRow->addWidget(m_categoryLabel);
    infoRow->addWidget(m_descriptionLabel, 1);
    selectLayout->addLayout(infoRow);

    mainLayout->addWidget(selectGroup);

    QGroupBox* paramGroup = new QGroupBox(tr("参数"));
    QVBoxLayout* paramLayout = new QVBoxLayout(paramGroup);

    m_paramScrollArea = new QScrollArea();
    m_paramScrollArea->setWidgetResizable(true);
    m_paramScrollArea->setMinimumHeight(120);
    m_paramScrollArea->setFrameShape(QFrame::NoFrame);
    m_paramContainer = new QWidget();
    m_paramScrollArea->setWidget(m_paramContainer);
    paramLayout->addWidget(m_paramScrollArea);

    mainLayout->addWidget(paramGroup);

    QGroupBox* progressGroup = new QGroupBox(tr("执行状态"));
    QVBoxLayout* progressLayout = new QVBoxLayout(progressGroup);

    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    progressLayout->addWidget(m_progressBar);

    QHBoxLayout* progressInfoRow = new QHBoxLayout();
    m_progressLabel = new QLabel(tr("就绪"));
    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet("color: #666;");
    progressInfoRow->addWidget(m_progressLabel, 1);
    progressInfoRow->addWidget(m_statusLabel);
    progressLayout->addLayout(progressInfoRow);

    mainLayout->addWidget(progressGroup);

    QGroupBox* resultGroup = new QGroupBox(tr("结果"));
    QVBoxLayout* resultLayout = new QVBoxLayout(resultGroup);

    m_resultTextEdit = new QTextEdit();
    m_resultTextEdit->setReadOnly(true);
    m_resultTextEdit->setPlaceholderText(tr("执行结果将显示在这里"));
    resultLayout->addWidget(m_resultTextEdit);

    QHBoxLayout* resultButtonRow = new QHBoxLayout();
    m_copyResultButton = new QPushButton(tr("复制结果"));
    m_exportResultButton = new QPushButton(tr("导出结果"));
    m_applyResultButton = new QPushButton(tr("应用到当前任务"));
    m_copyResultButton->setEnabled(false);
    m_exportResultButton->setEnabled(false);
    m_applyResultButton->setEnabled(false);
    resultButtonRow->addWidget(m_copyResultButton);
    resultButtonRow->addWidget(m_exportResultButton);
    resultButtonRow->addWidget(m_applyResultButton);
    resultButtonRow->addStretch();
    resultLayout->addLayout(resultButtonRow);

    mainLayout->addWidget(resultGroup);

    QHBoxLayout* buttonRow = new QHBoxLayout();
    buttonRow->addStretch();
    m_executeButton = new QPushButton(tr("执行"));
    m_executeButton->setDefault(true);
    m_cancelButton = new QPushButton(tr("关闭"));
    buttonRow->addWidget(m_executeButton);
    buttonRow->addWidget(m_cancelButton);
    mainLayout->addLayout(buttonRow);
}

void SkillInvokeDialog::setupConnections() {
    connect(m_skillComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SkillInvokeDialog::onSkillSelected);
    connect(m_executeButton, &QPushButton::clicked, this, &SkillInvokeDialog::onExecuteClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &SkillInvokeDialog::onCancelClicked);
    connect(m_copyResultButton, &QPushButton::clicked, this, &SkillInvokeDialog::onCopyResultClicked);
    connect(m_exportResultButton, &QPushButton::clicked, this, &SkillInvokeDialog::onExportResultClicked);
    connect(m_applyResultButton, &QPushButton::clicked, this, &SkillInvokeDialog::onApplyResultClicked);

    SkillManager* sm = SkillManager::instance();
    connect(sm, &SkillManager::skillStarted, this, &SkillInvokeDialog::onSkillStarted);
    connect(sm, &SkillManager::skillProgress, this, &SkillInvokeDialog::onSkillProgress);
    connect(sm, &SkillManager::skillFinished, this, &SkillInvokeDialog::onSkillFinished);
    connect(sm, &SkillManager::skillError, this, &SkillInvokeDialog::onSkillError);
}

void SkillInvokeDialog::populateSkillList() {
    m_skillComboBox->clear();
    QStringList skills = SkillManager::instance()->availableSkills();
    for (const QString& skill : skills) {
        QString desc = SkillManager::instance()->skillDescription(skill);
        m_skillComboBox->addItem(QString("%1 - %2").arg(skill).arg(desc), skill);
    }
    if (!skills.isEmpty()) {
        onSkillSelected(0);
    }
}

void SkillInvokeDialog::onSkillSelected(int index) {
    if (index < 0) return;

    QString skillName = m_skillComboBox->itemData(index).toString();
    SkillInfo info = SkillManager::instance()->skillInfo(skillName);

    m_categoryLabel->setText(QString("[%1]").arg(info.category));
    m_descriptionLabel->setText(info.description);

    clearParamWidgets();
    updateParamWidgets();
}

void SkillInvokeDialog::updateParamWidgets() {
    QString skillName = m_skillComboBox->currentData().toString();
    if (skillName.isEmpty()) return;

    SkillInfo info = SkillManager::instance()->skillInfo(skillName);
    QVariantMap schema = info.paramSchema;

    QVBoxLayout* layout = new QVBoxLayout(m_paramContainer);
    layout->setSpacing(8);

    QStringList paramNames = schema.keys();
    std::sort(paramNames.begin(), paramNames.end());

    for (const QString& paramName : paramNames) {
        QVariantMap paramDef = schema[paramName].toMap();
        QString type = paramDef.value("type", "string").toString();
        QString desc = paramDef.value("description", "").toString();
        bool required = paramDef.value("required", false).toBool();
        QVariant defaultValue = paramDef.value("default", QVariant());

        QLabel* label = new QLabel(QString("%1%2").arg(paramName).arg(required ? " *" : ""));
        label->setToolTip(desc);

        QWidget* inputWidget = nullptr;

        if (type == "string") {
            QLineEdit* lineEdit = new QLineEdit();
            if (defaultValue.isValid()) {
                lineEdit->setText(defaultValue.toString());
            }
            if (paramName == "file" || paramName == "output_file" || paramName == "directory") {
                lineEdit->setPlaceholderText(tr("点击选择..."));
                lineEdit->setProperty("isPath", true);
                lineEdit->installEventFilter(this);
            }
            inputWidget = lineEdit;
        }
        else if (type == "bool") {
            QCheckBox* checkBox = new QCheckBox(desc);
            if (defaultValue.isValid()) {
                checkBox->setChecked(defaultValue.toBool());
            }
            inputWidget = checkBox;
        }
        else if (type == "int") {
            QSpinBox* spinBox = new QSpinBox();
            spinBox->setRange(-1000000, 1000000);
            if (defaultValue.isValid()) {
                spinBox->setValue(defaultValue.toInt());
            }
            inputWidget = spinBox;
        }
        else if (type == "array") {
            QLineEdit* lineEdit = new QLineEdit();
            lineEdit->setPlaceholderText(tr("多项用逗号分隔"));
            if (!m_preselectedFiles.isEmpty() && paramName == "files") {
                lineEdit->setText(m_preselectedFiles.join(","));
            }
            inputWidget = lineEdit;
        }

        if (inputWidget) {
            QHBoxLayout* rowLayout = new QHBoxLayout();
            rowLayout->addWidget(label);
            rowLayout->addWidget(inputWidget, 1);
            layout->addLayout(rowLayout);

            ParamWidget pw;
            pw.name = paramName;
            pw.widget = inputWidget;
            pw.type = type;
            m_paramWidgets.append(pw);
        }
    }

    layout->addStretch();
    m_paramContainer->setLayout(layout);
}

void SkillInvokeDialog::clearParamWidgets() {
    m_paramWidgets.clear();
    if (m_paramContainer->layout()) {
        QLayoutItem* item;
        while ((item = m_paramContainer->layout()->takeAt(0)) != nullptr) {
            if (item->widget()) {
                delete item->widget();
            }
            delete item;
        }
        delete m_paramContainer->layout();
    }
}

QVariantMap SkillInvokeDialog::collectParams() {
    QVariantMap params;
    for (const ParamWidget& pw : m_paramWidgets) {
        if (pw.type == "string") {
            QLineEdit* lineEdit = qobject_cast<QLineEdit*>(pw.widget);
            if (lineEdit) {
                params[pw.name] = lineEdit->text();
            }
        }
        else if (pw.type == "bool") {
            QCheckBox* checkBox = qobject_cast<QCheckBox*>(pw.widget);
            if (checkBox) {
                params[pw.name] = checkBox->isChecked();
            }
        }
        else if (pw.type == "int") {
            QSpinBox* spinBox = qobject_cast<QSpinBox*>(pw.widget);
            if (spinBox) {
                params[pw.name] = spinBox->value();
            }
        }
        else if (pw.type == "array") {
            QLineEdit* lineEdit = qobject_cast<QLineEdit*>(pw.widget);
            if (lineEdit) {
                QStringList items = lineEdit->text().split(',', Qt::SkipEmptyParts);
                for (QString& item : items) {
                    item = item.trimmed();
                }
                params[pw.name] = items;
            }
        }
    }
    return params;
}

void SkillInvokeDialog::onExecuteClicked() {
    if (m_isExecuting) {
        SkillManager::instance()->cancelSkill(m_currentSkillId);
        return;
    }

    QString skillName = m_skillComboBox->currentData().toString();
    if (skillName.isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("请选择一个 Skill"));
        return;
    }

    QVariantMap params = collectParams();

    SkillInfo info = SkillManager::instance()->skillInfo(skillName);
    for (auto it = info.paramSchema.begin(); it != info.paramSchema.end(); ++it) {
        QVariantMap paramDef = it.value().toMap();
        bool required = paramDef.value("required", false).toBool();
        if (required && !params.contains(it.key())) {
            QMessageBox::warning(this, tr("警告"), tr("参数 '%1' 是必需的").arg(it.key()));
            return;
        }
        if (required && params[it.key()].toString().isEmpty()) {
            QMessageBox::warning(this, tr("警告"), tr("参数 '%1' 不能为空").arg(it.key()));
            return;
        }
    }

    m_currentSkillName = skillName;
    m_resultTextEdit->clear();
    m_progressBar->setValue(0);
    m_progressLabel->setText(tr("正在执行..."));

    setExecuting(true);
    SkillManager::instance()->invokeSkill(skillName, params);

    LOG_INFO("SkillInvokeDialog", QString("执行Skill: %1").arg(skillName));
}

void SkillInvokeDialog::onCancelClicked() {
    if (m_isExecuting) {
        SkillManager::instance()->cancelSkill(m_currentSkillId);
    }
    reject();
}

void SkillInvokeDialog::onCopyResultClicked() {
    if (!m_lastResult.isValid()) return;

    QString resultText = formatResult(m_lastResult);
    QApplication::clipboard()->setText(resultText);
    QMessageBox::information(this, tr("成功"), tr("结果已复制到剪贴板"));
}

void SkillInvokeDialog::onExportResultClicked() {
    if (!m_lastResult.isValid()) return;

    QString fileName = QFileDialog::getSaveFileName(this, tr("导出结果"),
        QString("%1_result.json").arg(m_currentSkillName),
        tr("JSON 文件 (*.json);;文本文件 (*.txt)"));

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("错误"), tr("无法写入文件"));
        return;
    }

    if (fileName.endsWith(".json")) {
        QJsonDocument doc = QJsonDocument::fromVariant(m_lastResult);
        file.write(doc.toJson(QJsonDocument::Indented));
    } else {
        file.write(formatResult(m_lastResult).toUtf8());
    }

    file.close();
    QMessageBox::information(this, tr("成功"), tr("结果已导出到: %1").arg(fileName));
    LOG_INFO("SkillInvokeDialog", QString("导出结果到: %1").arg(fileName));
}

void SkillInvokeDialog::onApplyResultClicked() {
    emit skillResultReady(m_currentSkillName, m_lastResult);
    QMessageBox::information(this, tr("成功"), tr("结果已应用到当前任务"));
    accept();
}

void SkillInvokeDialog::onSkillStarted(const QString& skillId, const QString& skillName) {
    Q_UNUSED(skillName);
    m_currentSkillId = skillId;
    m_statusLabel->setText(QString("ID: %1").arg(skillId.left(16)));
}

void SkillInvokeDialog::onSkillProgress(const QString& skillId, int progress, const QString& message) {
    if (skillId != m_currentSkillId) return;

    m_progressBar->setValue(progress);
    if (!message.isEmpty()) {
        m_progressLabel->setText(message);
    }
}

void SkillInvokeDialog::onSkillFinished(const QString& skillId, bool success, const QVariant& result) {
    if (skillId != m_currentSkillId) return;

    m_lastResult = result;
    setExecuting(false);

    if (success) {
        m_progressBar->setValue(100);
        m_progressLabel->setText(tr("执行成功"));
        displayResult(result);
        m_copyResultButton->setEnabled(true);
        m_exportResultButton->setEnabled(true);
        m_applyResultButton->setEnabled(true);
        LOG_INFO("SkillInvokeDialog", QString("Skill执行成功: %1").arg(m_currentSkillName));
    } else {
        m_progressLabel->setText(tr("执行失败"));
        if (result.canConvert<QVariantMap>()) {
            QVariantMap resultMap = result.toMap();
            m_resultTextEdit->setText(QString("错误: %1").arg(resultMap.value("error").toString()));
        } else {
            m_resultTextEdit->setText(QString("错误: %1").arg(result.toString()));
        }
        LOG_ERROR("SkillInvokeDialog", QString("Skill执行失败: %1").arg(m_currentSkillName));
    }
}

void SkillInvokeDialog::onSkillError(const QString& skillId, const QString& error) {
    if (skillId != m_currentSkillId) return;

    setExecuting(false);
    m_progressLabel->setText(tr("执行出错"));
    m_resultTextEdit->setText(QString("错误: %1").arg(error));
    LOG_ERROR("SkillInvokeDialog", QString("Skill执行错误: %1").arg(error));
}

void SkillInvokeDialog::displayResult(const QVariant& result) {
    QString formatted = formatResult(result);
    m_resultTextEdit->setText(formatted);
}

QString SkillInvokeDialog::formatResult(const QVariant& result, int indent) {
    QString prefix = QString("  ").repeated(indent);

    if (result.canConvert<QVariantMap>()) {
        QVariantMap map = result.toMap();
        QStringList lines;
        lines.append("{");
        QStringList keys = map.keys();
        for (const QString& key : keys) {
            lines.append(QString("%1  \"%2\": %3,").arg(prefix).arg(key).arg(formatResult(map[key], indent + 1)));
        }
        if (!keys.isEmpty()) {
            QString& last = lines.last();
            last.chop(1);
        }
        lines.append(prefix + "}");
        return lines.join("\n");
    }
    else if (result.canConvert<QVariantList>()) {
        QVariantList list = result.toList();
        QStringList lines;
        lines.append("[");
        for (const QVariant& item : list) {
            lines.append(QString("%1  %2,").arg(prefix).arg(formatResult(item, indent + 1)));
        }
        if (!list.isEmpty()) {
            QString& last = lines.last();
            last.chop(1);
        }
        lines.append(prefix + "]");
        return lines.join("\n");
    }
    else if (result.canConvert<QString>()) {
        return QString("\"%1\"").arg(result.toString());
    }
    else if (result.canConvert<bool>() && !result.canConvert<int>()) {
        return result.toBool() ? "true" : "false";
    }
    else if (result.canConvert<int>()) {
        return QString::number(result.toInt());
    }
    else {
        return result.toString();
    }
}

void SkillInvokeDialog::setExecuting(bool executing) {
    m_isExecuting = executing;
    m_executeButton->setText(executing ? tr("取消") : tr("执行"));
    m_skillComboBox->setEnabled(!executing);
    m_paramScrollArea->setEnabled(!executing);
}

void SkillInvokeDialog::setPreselectedFiles(const QStringList& files) {
    m_preselectedFiles = files;
    updateParamWidgets();
}
