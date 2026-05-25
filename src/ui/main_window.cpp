#include "main_window.h"
#include "task_list_widget.h"
#include "config_panel.h"
#include "file_list_widget.h"
#include "progress_widget.h"
#include "batch_conversion_summary.h"
#include "task_manager.h"
#include "config_manager.h"
#include "logger.h"
#include "error_types.h"
#include "format_registry.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QHBoxLayout>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_fileListWidget(nullptr)
    , m_taskListWidget(nullptr)
    , m_configPanel(nullptr)
    , m_progressWidget(nullptr)
    , m_mainSplitter(nullptr)
    , m_statusLabel(nullptr)
    , m_taskStatsLabel(nullptr)
    , m_startAction(nullptr)
    , m_pauseAction(nullptr)
    , m_resumeAction(nullptr)
    , m_cancelAction(nullptr)
    , m_summaryAction(nullptr)
    , m_isPaused(false)
    , m_darkMode(false)
{
    setWindowTitle(tr("集成格式转换工具 v1.0"));
    resize(1200, 800);
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupCentralWidget();
    setupConnections();
    applyLightTheme();
    LOG_INFO("MainWindow", "主窗口初始化完成");
}

MainWindow::~MainWindow() {
}

void MainWindow::setupMenuBar() {
    QMenuBar* menuBar = this->menuBar();
    QMenu* fileMenu = menuBar->addMenu(tr("文件(&F)"));
    QAction* addFilesAction = fileMenu->addAction(tr("添加文件(&A)"));
    addFilesAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));
    connect(addFilesAction, &QAction::triggered, this, &MainWindow::onAddFiles);
    QAction* addFolderAction = fileMenu->addAction(tr("添加文件夹(&D)"));
    addFolderAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O));
    connect(addFolderAction, &QAction::triggered, this, &MainWindow::onAddFolder);
    fileMenu->addSeparator();
    QAction* exitAction = fileMenu->addAction(tr("退出(&X)"));
    exitAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_F4));
    connect(exitAction, &QAction::triggered, this, &MainWindow::onExit);

    QMenu* toolMenu = menuBar->addMenu(tr("工具(&T)"));
    m_startAction = toolMenu->addAction(tr("开始转换(&S)"));
    m_startAction->setShortcut(QKeySequence(Qt::Key_F5));
    connect(m_startAction, &QAction::triggered, this, &MainWindow::onStartConversion);
    m_pauseAction = toolMenu->addAction(tr("暂停(&P)"));
    m_pauseAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
    m_pauseAction->setEnabled(false);
    connect(m_pauseAction, &QAction::triggered, this, &MainWindow::onPauseConversion);
    m_resumeAction = toolMenu->addAction(tr("继续(&R)"));
    m_resumeAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    m_resumeAction->setEnabled(false);
    connect(m_resumeAction, &QAction::triggered, this, &MainWindow::onResumeConversion);
    toolMenu->addSeparator();
    m_cancelAction = toolMenu->addAction(tr("取消全部(&C)"));
    m_cancelAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_C));
    connect(m_cancelAction, &QAction::triggered, this, &MainWindow::onCancelAll);
    toolMenu->addSeparator();
    m_summaryAction = toolMenu->addAction(tr("查看汇总(&V)"));
    m_summaryAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
    m_summaryAction->setEnabled(false);
    connect(m_summaryAction, &QAction::triggered, this, &MainWindow::onShowSummary);

    QMenu* viewMenu = menuBar->addMenu(tr("视图(&V)"));
    QAction* themeAction = viewMenu->addAction(tr("切换深色模式(&D)"));
    themeAction->setCheckable(true);
    themeAction->setChecked(false);
    connect(themeAction, &QAction::triggered, this, &MainWindow::toggleTheme);

    QMenu* helpMenu = menuBar->addMenu(tr("帮助(&H)"));
    QAction* aboutAction = helpMenu->addAction(tr("关于(&A)"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::setupToolBar() {
    QToolBar* toolBar = addToolBar(tr("主工具栏"));
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(24, 24));

    QAction* addFilesAction = toolBar->addAction(style()->standardIcon(QStyle::SP_FileIcon), tr("添加文件"));
    connect(addFilesAction, &QAction::triggered, this, &MainWindow::onAddFiles);

    QAction* addFolderAction = toolBar->addAction(style()->standardIcon(QStyle::SP_DirIcon), tr("添加文件夹"));
    connect(addFolderAction, &QAction::triggered, this, &MainWindow::onAddFolder);

    toolBar->addSeparator();

    m_toolbarStartAction = toolBar->addAction(style()->standardIcon(QStyle::SP_MediaPlay), tr("开始转换"));
    connect(m_toolbarStartAction, &QAction::triggered, this, &MainWindow::onStartConversion);
}

void MainWindow::setupStatusBar() {
    QStatusBar* statusBar = this->statusBar();
    m_statusLabel = new QLabel(tr("就绪"));
    m_statusLabel->setStyleSheet("padding: 2px 8px;");
    m_taskStatsLabel = new QLabel();
    m_taskStatsLabel->setStyleSheet("padding: 2px 8px;");
    statusBar->addWidget(m_statusLabel, 1);
    statusBar->addPermanentWidget(m_taskStatsLabel);
}

void MainWindow::setupCentralWidget() {
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    QWidget* topPanel = new QWidget();
    QHBoxLayout* topLayout = new QHBoxLayout(topPanel);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(8);

    QGroupBox* fileListGroup = new QGroupBox(tr("文件列表"));
    fileListGroup->setStyleSheet(
        "QGroupBox { font-weight: bold; border: 1px solid #ccc; border-radius: 4px; margin-top: 8px; padding-top: 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }"
    );
    QVBoxLayout* fileListLayout = new QVBoxLayout(fileListGroup);
    m_fileListWidget = new FileListWidget();
    fileListLayout->addWidget(m_fileListWidget);
    topLayout->addWidget(fileListGroup, 2);

    QGroupBox* configGroup = new QGroupBox();
    configGroup->setStyleSheet(
        "QGroupBox { font-weight: bold; border: 2px solid #2196F3; border-radius: 6px; background-color: #f8fbff; }"
    );
    QVBoxLayout* configLayout = new QVBoxLayout(configGroup);
    configLayout->setContentsMargins(0, 0, 0, 0);
    m_configPanel = new ConfigPanel();
    configLayout->addWidget(m_configPanel);
    topLayout->addWidget(configGroup, 1);

    mainLayout->addWidget(topPanel, 2);

    QGroupBox* taskListGroup = new QGroupBox(tr("任务列表"));
    taskListGroup->setStyleSheet(
        "QGroupBox { font-weight: bold; border: 1px solid #ccc; border-radius: 4px; margin-top: 8px; padding-top: 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }"
    );
    QVBoxLayout* taskListLayout = new QVBoxLayout(taskListGroup);
    m_progressWidget = new ProgressWidget();
    taskListLayout->addWidget(m_progressWidget);
    m_taskListWidget = new TaskListWidget();
    taskListLayout->addWidget(m_taskListWidget, 1);
    mainLayout->addWidget(taskListGroup, 1);

    setCentralWidget(centralWidget);
}

void MainWindow::setupConnections() {
    TaskManager* tm = TaskManager::instance();
    connect(tm, &TaskManager::taskAdded, this, &MainWindow::onTaskAdded);
    connect(tm, &TaskManager::taskStarted, this, &MainWindow::onTaskStarted);
    connect(tm, &TaskManager::taskProgressChanged, this, &MainWindow::onTaskProgressChanged);
    connect(tm, &TaskManager::taskCompleted, this, &MainWindow::onTaskCompleted);
    connect(tm, &TaskManager::allTasksCompleted, this, &MainWindow::onAllTasksCompleted);
    connect(m_fileListWidget, &FileListWidget::filesAdded, this, &MainWindow::onFilesAdded);
    connect(m_fileListWidget, &FileListWidget::fileCountChanged, this, &MainWindow::onFileCountChanged);
}

void MainWindow::toggleTheme() {
    m_darkMode = !m_darkMode;
    if (m_darkMode) {
        applyDarkTheme();
    } else {
        applyLightTheme();
    }
}

void MainWindow::applyLightTheme() {
    QString style = R"(
        QMainWindow { background-color: #f5f5f5; }
        QMenuBar { background-color: #ffffff; border-bottom: 1px solid #e0e0e0; }
        QMenuBar::item { padding: 6px 12px; background: transparent; }
        QMenuBar::item:selected { background-color: #e3f2fd; }
        QMenu { background-color: #ffffff; border: 1px solid #e0e0e0; }
        QMenu::item { padding: 6px 30px 6px 20px; }
        QMenu::item:selected { background-color: #e3f2fd; }
        QToolBar { background-color: #ffffff; border-bottom: 1px solid #e0e0e0; spacing: 5px; padding: 4px; }
        QToolBar QToolButton { padding: 6px; border-radius: 4px; border: 1px solid transparent; }
        QToolBar QToolButton:hover { background-color: #e3f2fd; }
        QStatusBar { background-color: #ffffff; border-top: 1px solid #e0e0e0; }
        QGroupBox { font-weight: bold; border: 1px solid #ccc; border-radius: 4px; margin-top: 8px; padding-top: 8px; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }
        QTableWidget { border: 1px solid #ccc; border-radius: 4px; gridline-color: #e0e0e0; background: #fff; }
        QTableWidget::item:selected { background-color: #e3f2fd; color: #000; }
        QHeaderView::section { background-color: #f5f5f5; border: none; border-bottom: 1px solid #ccc; padding: 4px; font-weight: bold; }
        QPushButton { padding: 6px 12px; border: 1px solid #ccc; border-radius: 4px; background-color: #fff; }
        QPushButton:hover { background-color: #f0f0f0; }
        QPushButton:disabled { background-color: #f5f5f5; color: #999; }
        QCheckBox { spacing: 6px; }
        QLabel { color: #333; }
    )";
    setStyleSheet(style);
}

void MainWindow::applyDarkTheme() {
    QString style = R"(
        QMainWindow { background-color: #1e1e1e; }
        QMenuBar { background-color: #2d2d2d; border-bottom: 1px solid #3c3c3c; color: #e0e0e0; }
        QMenuBar::item { padding: 6px 12px; background: transparent; color: #e0e0e0; }
        QMenuBar::item:selected { background-color: #094771; }
        QMenu { background-color: #2d2d2d; border: 1px solid #3c3c3c; color: #e0e0e0; }
        QMenu::item { padding: 6px 30px 6px 20px; color: #e0e0e0; }
        QMenu::item:selected { background-color: #094771; }
        QToolBar { background-color: #2d2d2d; border-bottom: 1px solid #3c3c3c; spacing: 5px; padding: 4px; }
        QToolBar QToolButton { padding: 6px; border-radius: 4px; border: 1px solid transparent; color: #e0e0e0; }
        QToolBar QToolButton:hover { background-color: #094771; }
        QStatusBar { background-color: #2d2d2d; border-top: 1px solid #3c3c3c; color: #e0e0e0; }
        QGroupBox { font-weight: bold; border: 1px solid #3c3c3c; border-radius: 4px; margin-top: 8px; padding-top: 8px; color: #e0e0e0; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; color: #e0e0e0; }
        QTableWidget { border: 1px solid #3c3c3c; border-radius: 4px; gridline-color: #3c3c3c; background: #252526; color: #e0e0e0; }
        QTableWidget::item:selected { background-color: #094771; color: #fff; }
        QHeaderView::section { background-color: #2d2d2d; border: none; border-bottom: 1px solid #3c3c3c; padding: 4px; font-weight: bold; color: #e0e0e0; }
        QPushButton { padding: 6px 12px; border: 1px solid #3c3c3c; border-radius: 4px; background-color: #333; color: #e0e0e0; }
        QPushButton:hover { background-color: #094771; }
        QPushButton:disabled { background-color: #2d2d2d; color: #666; }
        QCheckBox { spacing: 6px; color: #e0e0e0; }
        QLabel { color: #e0e0e0; }
        QComboBox { padding: 8px 12px; border: 2px solid #0078d4; border-radius: 8px; min-width: 120px; background-color: #333; color: #e0e0e0; font-size: 14px; }
        QComboBox::drop-down { border: none; width: 32px; }
        QComboBox QAbstractItemView { border: 1px solid #3c3c3c; border-radius: 4px; background-color: #2d2d2d; color: #e0e0e0; selection-background-color: #094771; font-size: 13px; }
        QLineEdit { padding: 8px 12px; border: 1px solid #3c3c3c; border-radius: 6px; background-color: #333; color: #e0e0e0; font-size: 13px; }
        QScrollBar:vertical { background: #2d2d2d; width: 12px; }
        QScrollBar::handle:vertical { background: #555; border-radius: 6px; min-height: 30px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
    )";
    setStyleSheet(style);
}

void MainWindow::onAddFiles() {
    const auto& reg = FormatRegistry::instance();
    QStringList files = QFileDialog::getOpenFileNames(this, tr("选择文件"),
        QString(),
        tr("所有支持格式 (%1);;"
           "%2;;"
           "%3;;"
           "%4;;"
           "%5;;"
           "所有文件 (*)")
        .arg(reg.fileDialogFilter())
        .arg(reg.fileDialogImageFilter())
        .arg(reg.fileDialogVideoFilter())
        .arg(reg.fileDialogAudioFilter())
        .arg(reg.fileDialogDocumentFilter())
    );
    if (!files.isEmpty()) {
        m_fileListWidget->addFiles(files);
    }
}

void MainWindow::onAddFolder() {
    QString folder = QFileDialog::getExistingDirectory(this, tr("选择文件夹"));
    if (!folder.isEmpty()) {
        m_fileListWidget->addFolder(folder, true);
    }
}

void MainWindow::onStartConversion() {
    if (m_fileListWidget->isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先添加要转换的文件"));
        return;
    }
    m_conversionResults.clear();
    submitConversionTasks();
    TaskManager::instance()->start();
    m_statusLabel->setText(tr("正在转换..."));
    m_startAction->setEnabled(false);
    m_pauseAction->setEnabled(true);
    m_resumeAction->setEnabled(false);
    m_summaryAction->setEnabled(false);
    m_toolbarStartAction->setEnabled(false);
    LOG_INFO("MainWindow", "开始转换任务");
}

void MainWindow::onPauseConversion() {
    TaskManager::instance()->pause();
    m_isPaused = true;
    m_statusLabel->setText(tr("已暂停"));
    m_pauseAction->setEnabled(false);
    m_resumeAction->setEnabled(true);
    LOG_INFO("MainWindow", "暂停转换");
}

void MainWindow::onResumeConversion() {
    TaskManager::instance()->resume();
    m_isPaused = false;
    m_statusLabel->setText(tr("正在转换..."));
    m_pauseAction->setEnabled(true);
    m_resumeAction->setEnabled(false);
    LOG_INFO("MainWindow", "继续转换");
}

void MainWindow::onCancelAll() {
    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("确认"),
        tr("确定要取消所有任务吗？"), QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        TaskManager::instance()->cancelAllTasks();
        m_statusLabel->setText(tr("已取消"));
        m_startAction->setEnabled(true);
        m_pauseAction->setEnabled(false);
        m_resumeAction->setEnabled(false);
        m_toolbarStartAction->setEnabled(true);
        m_isPaused = false;
        LOG_INFO("MainWindow", "取消所有任务");
    }
}

void MainWindow::onAbout() {
    QMessageBox::about(this, tr("关于"),
        tr("<h3>集成格式转换工具 v1.0.0</h3>"
           "<p>基于FFmpeg、Pandoc和ImageMagick的多功能文件转换工具</p>"
           "<p>支持功能：</p>"
           "<ul>"
           "<li>视频格式转换（MP4, AVI, MKV等）</li>"
           "<li>音频格式转换（MP3, WAV, FLAC等）</li>"
           "<li>图片格式转换（PNG, JPG, GIF, WebP等）</li>"
           "<li>文档格式转换（Markdown, DOCX, PDF等）</li>"
           "</ul>"
           "<p> 2024 ConverterTools</p>"));
}

void MainWindow::onExit() {
    close();
}

void MainWindow::onTaskAdded(const QString& taskId) {
    Q_UNUSED(taskId);
    updateStatusBar();
    updateProgressWidget();
    m_taskListWidget->refreshTaskList();
}

void MainWindow::onTaskStarted(const QString& taskId) {
    ConversionTask* task = TaskManager::instance()->getTask(taskId);
    if (task) {
        m_currentConvertingFile = task->inputFile();
        QFileInfo fi(m_currentConvertingFile);
        m_progressWidget->setCurrentFile(fi.fileName());
    }
    updateStatusBar();
    updateProgressWidget();
}

void MainWindow::onTaskProgressChanged(const QString& taskId, int progress) {
    Q_UNUSED(taskId);
    Q_UNUSED(progress);
    updateProgressWidget();
}

void MainWindow::onTaskCompleted(const QString& taskId, bool success) {
    ConversionTask* task = TaskManager::instance()->getTask(taskId);
    if (task) {
        ConversionResult result;
        result.inputPath = task->inputFile();
        result.outputPath = task->outputFile();
        result.success = success;
        result.errorMessage = task->errorMessage();
        result.durationMs = task->durationMs();
        m_conversionResults.append(result);
    }
    updateStatusBar();
    updateProgressWidget();
    m_taskListWidget->refreshTaskList();
}

void MainWindow::onAllTasksCompleted() {
    m_statusLabel->setText(tr("所有任务已完成"));
    m_startAction->setEnabled(true);
    m_pauseAction->setEnabled(false);
    m_resumeAction->setEnabled(false);
    m_isPaused = false;
    m_summaryAction->setEnabled(!m_conversionResults.isEmpty());
    m_toolbarStartAction->setEnabled(true);
    m_progressWidget->setCurrentFile(QString());
    updateProgressWidget();
    m_taskListWidget->refreshTaskList();
    if (ConfigManager::instance().value("showNotification", true).toBool()) {
        int completed = TaskManager::instance()->completedCount();
        int failed = TaskManager::instance()->failedCount();
        QString msg = tr("转换完成！\n成功: %1\n失败: %2").arg(completed).arg(failed);
        if (failed > 0) {
            QMessageBox::warning(this, tr("转换完成"), msg);
        } else {
            QMessageBox::information(this, tr("转换完成"), msg);
        }
    }
    if (!m_conversionResults.isEmpty()) {
        showConversionSummary();
    }
    LOG_INFO("MainWindow", "所有任务已完成");
}

void MainWindow::onFilesAdded(const QList<FileInfo>& files) {
    Q_UNUSED(files);
    m_statusLabel->setText(tr("已添加 %1 个文件").arg(files.size()));
}

void MainWindow::onFileCountChanged(int count) {
    if (count == 0) {
        m_statusLabel->setText(tr("就绪"));
    }
}

void MainWindow::onShowSummary() {
    showConversionSummary();
}

void MainWindow::onRetryFailed(const QList<QString>& inputPaths) {
    if (inputPaths.isEmpty()) return;
    m_fileListWidget->clear();
    m_fileListWidget->addFiles(inputPaths);
    m_conversionResults.clear();
    m_startAction->setEnabled(false);
    onStartConversion();
}

void MainWindow::updateStatusBar() {
    TaskManager* tm = TaskManager::instance();
    int total = tm->totalTaskCount();
    int running = tm->runningCount();
    int pending = tm->pendingCount();
    int completed = tm->completedCount();
    int failed = tm->failedCount();
    m_taskStatsLabel->setText(
        tr("总计: %1 | 运行: %2 | 等待: %3 | 完成: %4 | 失败: %5")
        .arg(total).arg(running).arg(pending).arg(completed).arg(failed)
    );
}

void MainWindow::updateProgressWidget() {
    TaskManager* tm = TaskManager::instance();
    m_progressWidget->updateFromTaskManager(
        tm->totalTaskCount(),
        tm->pendingCount(),
        tm->runningCount(),
        tm->completedCount(),
        tm->failedCount()
    );
}

void MainWindow::submitConversionTasks() {
    QList<FileInfo> files = m_fileListWidget->allFiles();
    if (files.isEmpty()) return;
    QString outputFormat = m_configPanel->selectedOutputFormat();
    QString outputDir = m_configPanel->outputDirectory();
    QVariantMap baseParams = m_configPanel->conversionParams();
    const auto& reg = FormatRegistry::instance();
    for (const FileInfo& fileInfo : files) {
        QFileInfo fi(fileInfo.filePath);
        QString baseName = fi.completeBaseName();
        QString outputFile = outputDir + "/" + baseName + "." + outputFormat;
        // 如果输出路径和输入路径相同（同目录同格式），加 _converted 后缀避免原地覆盖
        if (QFileInfo(outputFile).absoluteFilePath() == QFileInfo(fileInfo.filePath).absoluteFilePath()) {
            outputFile = outputDir + "/" + baseName + "_converted." + outputFormat;
            LOG_WARNING("MainWindow", QString("输出路径与输入相同，自动重命名: %1").arg(outputFile));
        }
        QVariantMap params = baseParams;
        params["outputFormat"] = outputFormat;
        QString ext = fi.suffix().toLower();
        auto converterType = reg.converterForExt(ext);
        if (converterType == FormatRegistry::Converter::Pandoc) {
            params["converter"] = "Pandoc";
        } else if (converterType == FormatRegistry::Converter::ImageMagick) {
            params["converter"] = "ImageMagick";
        } else {
            params["converter"] = "FFmpeg";
        }
        TaskManager::instance()->addTask(fileInfo.filePath, outputFile, params);
    }
    LOG_INFO("MainWindow", QString("提交 %1 个转换任务").arg(files.size()));
}

void MainWindow::showConversionSummary() {
    BatchConversionSummary summary(this);
    summary.setResults(m_conversionResults);
    connect(&summary, &BatchConversionSummary::retryRequested,
            this, &MainWindow::onRetryFailed);
    summary.exec();
}

void MainWindow::onErrorOccurred(const ErrorInfo& error) {
    Q_UNUSED(error);
}

void MainWindow::onRetryTriggered(const QString& taskId, int retryCount) {
    Q_UNUSED(taskId);
    Q_UNUSED(retryCount);
}
