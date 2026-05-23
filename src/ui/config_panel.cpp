#include "config_panel.h"
#include "config_manager.h"
#include "logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QDir>
#include <QLabel>
#include <QFrame>

ConfigPanel::ConfigPanel(QWidget* parent)
    : QWidget(parent)
    , m_outputFormatCombo(nullptr)
    , m_outputDirEdit(nullptr)
    , m_browseOutputBtn(nullptr)
{
    setupUI();
    setupConnections();
    loadSettings();
    applyStyleSheet();
}

ConfigPanel::~ConfigPanel() {
}

void ConfigPanel::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(16);

    QLabel* titleLabel = new QLabel(tr("格式转换设置"));
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2196F3;");
    mainLayout->addWidget(titleLabel);

    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: #e0e0e0;");
    mainLayout->addWidget(line);

    QLabel* formatTitle = new QLabel(tr("目标格式"));
    formatTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: #333;");
    mainLayout->addWidget(formatTitle);

    QHBoxLayout* formatLayout = new QHBoxLayout();
    QLabel* arrowLabel = new QLabel(tr("转换为"));
    arrowLabel->setStyleSheet("font-size: 13px; color: #666;");
    formatLayout->addWidget(arrowLabel);

    m_outputFormatCombo = new QComboBox();
    m_outputFormatCombo->setMinimumWidth(220);
    m_outputFormatCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_outputFormatCombo->setStyleSheet("font-size: 14px; padding: 6px;");
    formatLayout->addWidget(m_outputFormatCombo, 1);
    formatLayout->addStretch();
    mainLayout->addLayout(formatLayout);

    QFrame* line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setStyleSheet("color: #e0e0e0;");
    mainLayout->addWidget(line2);

    QLabel* outputTitle = new QLabel(tr("输出位置"));
    outputTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: #333;");
    mainLayout->addWidget(outputTitle);

    QHBoxLayout* outputDirLayout = new QHBoxLayout();
    m_outputDirEdit = new QLineEdit();
    m_outputDirEdit->setPlaceholderText(tr("选择输出目录..."));
    outputDirLayout->addWidget(m_outputDirEdit, 1);

    m_browseOutputBtn = new QPushButton(tr("浏览"));
    m_browseOutputBtn->setFixedWidth(80);
    m_browseOutputBtn->setMinimumHeight(32);
    outputDirLayout->addWidget(m_browseOutputBtn);

    mainLayout->addLayout(outputDirLayout);
    mainLayout->addStretch();

    updateOutputFormats();
}

void ConfigPanel::setupConnections() {
    connect(m_browseOutputBtn, &QPushButton::clicked, this, &ConfigPanel::onBrowseOutputDir);
}

void ConfigPanel::applyStyleSheet() {
    QString styleSheet = R"(
        QComboBox {
            padding: 8px 12px;
            border: 2px solid #2196F3;
            border-radius: 8px;
            min-width: 160px;
            background-color: #ffffff;
            font-size: 14px;
        }
        QComboBox::drop-down {
            border: none;
            width: 32px;
        }
        QComboBox QAbstractItemView {
            border: 1px solid #ccc;
            border-radius: 4px;
            background-color: #ffffff;
            selection-background-color: #e3f2fd;
            font-size: 13px;
        }
        QLineEdit {
            padding: 8px 12px;
            border: 1px solid #ccc;
            border-radius: 6px;
            background-color: #ffffff;
            font-size: 13px;
        }
        QPushButton {
            padding: 6px 12px;
            border: 1px solid #2196F3;
            border-radius: 6px;
            background-color: #2196F3;
            color: white;
            font-size: 13px;
            font-weight: bold;
            min-height: 32px;
        }
        QPushButton:hover {
            background-color: #1976D2;
        }
        QLabel {
            padding: 2px 0;
        }
    )";
    setStyleSheet(styleSheet);
}

void ConfigPanel::updateOutputFormats() {
    m_outputFormatCombo->clear();
    m_outputFormatCombo->addItem(tr("MP4 (视频)"), "mp4");
    m_outputFormatCombo->addItem(tr("AVI (视频)"), "avi");
    m_outputFormatCombo->addItem(tr("MKV (视频)"), "mkv");
    m_outputFormatCombo->addItem(tr("MOV (视频)"), "mov");
    m_outputFormatCombo->addItem(tr("WebM (视频)"), "webm");
    m_outputFormatCombo->addItem(tr("MP3 (音频)"), "mp3");
    m_outputFormatCombo->addItem(tr("WAV (音频)"), "wav");
    m_outputFormatCombo->addItem(tr("FLAC (音频)"), "flac");
    m_outputFormatCombo->addItem(tr("AAC (音频)"), "aac");
    m_outputFormatCombo->addItem(tr("PDF (文档)"), "pdf");
    m_outputFormatCombo->addItem(tr("DOCX (文档)"), "docx");
    m_outputFormatCombo->addItem(tr("HTML (文档)"), "html");
    m_outputFormatCombo->addItem(tr("Markdown (文档)"), "md");
    m_outputFormatCombo->addItem(tr("EPUB (文档)"), "epub");
    m_outputFormatCombo->addItem(tr("TXT (文档)"), "txt");
}

void ConfigPanel::onOutputFormatChanged(int index) {
    Q_UNUSED(index);
}

void ConfigPanel::onBrowseOutputDir() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("选择输出目录"),
                                                    m_outputDirEdit->text());
    if (!dir.isEmpty()) {
        m_outputDirEdit->setText(dir);
    }
}

void ConfigPanel::loadSettings() {
    ConfigManager& cm = ConfigManager::instance();
    QString outputDir = cm.outputDirectory();
    if (outputDir.isEmpty()) {
        outputDir = QDir::homePath() + "/converted";
    }
    m_outputDirEdit->setText(outputDir);
}

void ConfigPanel::saveSettings() {
    ConfigManager& cm = ConfigManager::instance();
    cm.setOutputDirectory(m_outputDirEdit->text());
    QString configPath = QDir::homePath() + "/.integrated_converter/config.json";
    cm.saveConfig(configPath);
}

QString ConfigPanel::selectedOutputFormat() const {
    return m_outputFormatCombo->currentData().toString();
}

QString ConfigPanel::outputDirectory() const {
    QString dir = m_outputDirEdit->text();
    if (dir.isEmpty()) {
        dir = QDir::homePath() + "/converted";
    }
    QDir().mkpath(dir);
    return dir;
}

QVariantMap ConfigPanel::conversionParams() const {
    QVariantMap params;
    params["outputFormat"] = m_outputFormatCombo->currentData().toString();
    return params;
}
