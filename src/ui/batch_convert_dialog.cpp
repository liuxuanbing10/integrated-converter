#include "batch_convert_dialog.h"
#include "file_list_widget.h"
#include <QFileInfo>
#include <QHeaderView>
#include <QFileDialog>
#include <QDir>
BatchConvertDialog::BatchConvertDialog(QWidget* parent)
    : QDialog(parent)
    , m_fileListGroup(nullptr)
    , m_fileTable(nullptr)
    , m_fileCountLabel(nullptr)
    , m_outputGroup(nullptr)
    , m_outputDirEdit(nullptr)
    , m_browseButton(nullptr)
    , m_outputFormatCombo(nullptr)
    , m_keepStructureCheck(nullptr)
    , m_namingRuleCombo(nullptr)
    , m_overwriteCheck(nullptr)
    , m_startButton(nullptr)
    , m_cancelButton(nullptr)
{
    setWindowTitle(tr("批量转换确认"));
    resize(700, 500);
    setupUI();
    setupConnections();
}
BatchConvertDialog::~BatchConvertDialog() {
}
void BatchConvertDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    m_fileListGroup = new QGroupBox(tr("待转换文件"));
    QVBoxLayout* fileListLayout = new QVBoxLayout(m_fileListGroup);
    QHBoxLayout* countLayout = new QHBoxLayout();
    m_fileCountLabel = new QLabel(tr("共 0 个文件"));
    m_fileCountLabel->setStyleSheet("font-weight: 600; color: #1664ff;");
    countLayout->addWidget(m_fileCountLabel);
    countLayout->addStretch();
    fileListLayout->addLayout(countLayout);
    m_fileTable = new QTableWidget();
    m_fileTable->setColumnCount(3);
    m_fileTable->setHorizontalHeaderLabels({
        tr("文件名"), tr("格式"), tr("路径")
    });
    m_fileTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_fileTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_fileTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_fileTable->setColumnWidth(0, 200);
    m_fileTable->setColumnWidth(1, 60);
    m_fileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_fileTable->setAlternatingRowColors(true);
    m_fileTable->setStyleSheet(
        "QTableWidget {"
        "  border: 1px solid #dde2e9;"
        "  border-radius: 8px;"
        "  gridline-color: #eceded;"
        "}"
        "QHeaderView::section {"
        "  background-color: #f7f9fb;"
        "  border: none;"
        "  border-bottom: 1px solid #dde2e9;"
        "  padding: 4px;"
        "  font-weight: 600;"
        "  color: #4e5969;"
        "}"
    );
    fileListLayout->addWidget(m_fileTable);
    mainLayout->addWidget(m_fileListGroup, 1);
    m_outputGroup = new QGroupBox(tr("输出设置"));
    QVBoxLayout* outputLayout = new QVBoxLayout(m_outputGroup);
    QHBoxLayout* dirLayout = new QHBoxLayout();
    dirLayout->addWidget(new QLabel(tr("输出目录:")));
    m_outputDirEdit = new QLineEdit();
    m_outputDirEdit->setPlaceholderText(tr("选择输出目录..."));
    m_outputDirEdit->setText(QDir::homePath() + "/converted");
    dirLayout->addWidget(m_outputDirEdit, 1);
    m_browseButton = new QPushButton(tr("浏览..."));
    dirLayout->addWidget(m_browseButton);
    outputLayout->addLayout(dirLayout);
    QHBoxLayout* formatLayout = new QHBoxLayout();
    formatLayout->addWidget(new QLabel(tr("输出格式:")));
    m_outputFormatCombo = new QComboBox();
    m_outputFormatCombo->addItem(tr("MP4"), "mp4");
    m_outputFormatCombo->addItem(tr("MKV"), "mkv");
    m_outputFormatCombo->addItem(tr("MP3"), "mp3");
    m_outputFormatCombo->addItem(tr("WAV"), "wav");
    m_outputFormatCombo->addItem(tr("PDF"), "pdf");
    m_outputFormatCombo->addItem(tr("DOCX"), "docx");
    m_outputFormatCombo->addItem(tr("HTML"), "html");
    formatLayout->addWidget(m_outputFormatCombo);
    formatLayout->addStretch();
    outputLayout->addLayout(formatLayout);
    QHBoxLayout* optionsLayout = new QHBoxLayout();
    m_keepStructureCheck = new QCheckBox(tr("保持原目录结构"));
    optionsLayout->addWidget(m_keepStructureCheck);
    optionsLayout->addSpacing(20);
    optionsLayout->addWidget(new QLabel(tr("文件名规则:")));
    m_namingRuleCombo = new QComboBox();
    m_namingRuleCombo->addItem(tr("原名"), 0);
    m_namingRuleCombo->addItem(tr("原名_时间戳"), 1);
    m_namingRuleCombo->addItem(tr("序号"), 2);
    optionsLayout->addWidget(m_namingRuleCombo);
    optionsLayout->addStretch();
    outputLayout->addLayout(optionsLayout);
    m_overwriteCheck = new QCheckBox(tr("覆盖已存在的文件"));
    outputLayout->addWidget(m_overwriteCheck);
    mainLayout->addWidget(m_outputGroup);
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_startButton = new QPushButton(tr("开始转换"));
    m_startButton->setIcon(QIcon(":/icons/play.svg"));
    m_startButton->setStyleSheet(
        "QPushButton {"
        "  padding: 8px 20px;"
        "  background-color: #1ebf6f;"
        "  color: #ffffff;"
        "  border: none;"
        "  border-radius: 8px;"
        "  font-weight: 600;"
        "}"
        "QPushButton:hover { background-color: #16b35f; }"
    );
    m_cancelButton = new QPushButton(tr("取消"));
    m_cancelButton->setIcon(QIcon(":/icons/cancel.svg"));
    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_cancelButton);
    mainLayout->addLayout(buttonLayout);
    setStyleSheet(
        "QGroupBox {"
        "  font-weight: 600;"
        "  border: 1px solid #dde2e9;"
        "  border-radius: 8px;"
        "  margin-top: 8px;"
        "  padding-top: 8px;"
        "  color: #1d2129;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left: 10px;"
        "  padding: 0 5px;"
        "}"
        "QPushButton {"
        "  padding: 6px 12px;"
        "  border: 1px solid #dde2e9;"
        "  border-radius: 8px;"
        "  background-color: #ffffff;"
        "  color: #1d2129;"
        "}"
        "QPushButton:hover { background-color: #f7f9fb; }"
        "QLineEdit {"
        "  padding: 4px 8px;"
        "  border: 1px solid #dde2e9;"
        "  border-radius: 8px;"
        "}"
        "QComboBox {"
        "  padding: 4px 8px;"
        "  border: 1px solid #dde2e9;"
        "  border-radius: 8px;"
        "}"
        "QCheckBox { spacing: 6px; color: #1d2129; }"
    );
}
void BatchConvertDialog::setupConnections() {
    connect(m_browseButton, &QPushButton::clicked, this, &BatchConvertDialog::onBrowseOutputDir);
    connect(m_startButton, &QPushButton::clicked, this, &BatchConvertDialog::onStart);
    connect(m_cancelButton, &QPushButton::clicked, this, &BatchConvertDialog::onCancel);
}
void BatchConvertDialog::setFiles(const QList<FileInfo>& files) {
    m_files = files;
    updateFilePreview();
}
void BatchConvertDialog::updateFilePreview() {
    m_fileCountLabel->setText(tr("共 %1 个文件").arg(m_files.size()));
    m_fileTable->setRowCount(m_files.size());
    for (int i = 0; i < m_files.size(); ++i) {
        const FileInfo& info = m_files[i];
        m_fileTable->setItem(i, 0, new QTableWidgetItem(info.fileName));
        m_fileTable->setItem(i, 1, new QTableWidgetItem(info.format.toUpper()));
        m_fileTable->setItem(i, 2, new QTableWidgetItem(info.filePath));
    }
}
void BatchConvertDialog::onBrowseOutputDir() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("选择输出目录"),
                                                    m_outputDirEdit->text());
    if (!dir.isEmpty()) {
        m_outputDirEdit->setText(dir);
    }
}
void BatchConvertDialog::onStart() {
    if (m_files.isEmpty()) {
        reject();
        return;
    }
    QString outputDir = m_outputDirEdit->text();
    if (outputDir.isEmpty()) {
        outputDir = QDir::homePath() + "/converted";
    }
    QDir().mkpath(outputDir);
    emit startConversionRequested();
    accept();
}
void BatchConvertDialog::onCancel() {
    reject();
}
QString BatchConvertDialog::outputDirectory() const {
    QString dir = m_outputDirEdit->text();
    if (dir.isEmpty()) {
        dir = QDir::homePath() + "/converted";
    }
    return dir;
}
QString BatchConvertDialog::outputFormat() const {
    return m_outputFormatCombo->currentData().toString();
}
bool BatchConvertDialog::keepDirectoryStructure() const {
    return m_keepStructureCheck->isChecked();
}
int BatchConvertDialog::namingRule() const {
    return m_namingRuleCombo->currentData().toInt();
}
bool BatchConvertDialog::overwriteExisting() const {
    return m_overwriteCheck->isChecked();
}
