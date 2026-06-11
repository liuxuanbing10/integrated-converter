#include "main_window.h"
#include "task_list_widget.h"
#include "file_list_widget.h"
#include "file_category_widget.h"
#include "progress_widget.h"
#include "batch_conversion_summary.h"
#include "conversion_params_dialog.h"
#include "task_manager.h"
#include "config_manager.h"
#include "logger.h"
#include "error_types.h"
#include "format_registry.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QDir>
#include <QFileInfo>
#include <QStyle>
#include <QStyleHints>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_tabWidget(nullptr)
    , m_imageTab(nullptr)
    , m_docTab(nullptr)
    , m_audioTab(nullptr)
    , m_videoTab(nullptr)
    , m_configPanel(nullptr)
    , m_formatCombo(nullptr)
    , m_outputDirEdit(nullptr)
    , m_convertBtn(nullptr)
    , m_paramsBtn(nullptr)
    , m_taskListWidget(nullptr)
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
    , m_lastActiveCategory(FormatRegistry::Category::Image)
{
    setWindowTitle(tr("集成格式转换工具 v1.3.1"));
    resize(1200, 800);
    setAcceptDrops(true);  // Enable drag-and-drop of files onto the main window
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupCentralWidget();
    setupConnections();
    applyLightTheme();

    // Qt 6.12: Use QStyleHints for fine-grained tooltip control
    if (auto* hints = QApplication::styleHints()) {
        hints->setToolTipWakeUpDelay(500);
    }

    LOG_INFO("MainWindow", "主窗口初始化完成 (外部配置面板)");
}

MainWindow::~MainWindow() {
}

void MainWindow::setupMenuBar() {
    QMenuBar* menuBar = this->menuBar();
    QMenu* fileMenu = menuBar->addMenu(tr("文件(&F)"));
    QAction* addFilesAction = fileMenu->addAction(tr("添加文件(&A)"));
    addFilesAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));
    connect(addFilesAction, &QAction::triggered, this, &MainWindow::onAddFiles);
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

    // ── Global action bar (above tabs) ────────────────────────────
    QHBoxLayout* globalBar = new QHBoxLayout();
    QPushButton* globalAddBtn = new QPushButton(style()->standardIcon(QStyle::SP_FileIcon),
        tr(" 选择文件（自动识别分类）"));
    globalAddBtn->setStyleSheet(
        "QPushButton { padding: 10px 24px; border: 2px solid #1976D2; "
        "border-radius: 8px; background-color: #1976D2; color: white; "
        "font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: #1565C0; }"
    );
    connect(globalAddBtn, &QPushButton::clicked, this, &MainWindow::onAddFiles);

    QLabel* globalHint = new QLabel(tr("支持图片、文档、音频、视频文件，系统自动识别分类"));
    globalHint->setStyleSheet("color: #666; font-size: 12px; padding-left: 8px;");

    globalBar->addWidget(globalAddBtn);
    globalBar->addWidget(globalHint, 1);
    mainLayout->addLayout(globalBar);

    // ── Content area: tabs (left) + config panel (right) ──────────
    QHBoxLayout* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(8);

    // -- Tab widget (left, stretchy) --
    m_tabWidget = new QTabWidget();
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #ccc; border-radius: 4px; "
        "background-color: #fafafa; padding: 0px; }"
        "QTabBar::tab { padding: 8px 20px; font-size: 13px; font-weight: bold; "
        "border: 1px solid #ccc; border-bottom: none; border-top-left-radius: 6px; "
        "border-top-right-radius: 6px; margin-right: 2px; }"
        "QTabBar::tab:selected { background-color: #fff; color: #1976D2; "
        "border-bottom: 2px solid #1976D2; }"
        "QTabBar::tab:!selected { background-color: #f0f0f0; color: #666; }"
        "QTabBar::tab:hover:!selected { background-color: #e3f2fd; }"
    );

    using Cat = FormatRegistry::Category;
    m_imageTab = new FileCategoryWidget(Cat::Image);
    m_docTab   = new FileCategoryWidget(Cat::Document);
    m_audioTab = new FileCategoryWidget(Cat::Audio);
    m_videoTab = new FileCategoryWidget(Cat::Video);

    m_tabWidget->addTab(m_imageTab, QString::fromUtf8("\xF0\x9F\x96\xBC") + tr(" 图片转换"));
    m_tabWidget->addTab(m_docTab,   QString::fromUtf8("\xF0\x9F\x93\x84") + tr(" 文档转换"));
    m_tabWidget->addTab(m_audioTab, QString::fromUtf8("\xF0\x9F\x8E\xB5") + tr(" 音频转换"));
    m_tabWidget->addTab(m_videoTab, QString::fromUtf8("\xF0\x9F\x8E\xAC") + tr(" 视频转换"));

    contentLayout->addWidget(m_tabWidget, 1);

    // -- Config panel (right, fixed width) --
    m_configPanel = new QFrame();
    m_configPanel->setObjectName("configPanel");
    m_configPanel->setFixedWidth(280);
    m_configPanel->setStyleSheet(
        "#configPanel { border: 1px solid #bbb; border-radius: 8px; "
        "background-color: #ffffff; }"
        "#configPanel QLabel { color: #333; background: transparent; border: none; }"
        "#configTitleTxt { color: #1565C0; font-size: 14px; font-weight: bold; }"
        "#configFormatLabel, #configDirLabel { font-size: 12px; font-weight: bold; }"
    );

    QVBoxLayout* configLayout = new QVBoxLayout(m_configPanel);
    configLayout->setContentsMargins(12, 14, 12, 12);
    configLayout->setSpacing(8);

    // Title
    QLabel* configTitle = new QLabel(tr("⚙ 转换设置"));
    configTitle->setObjectName("configTitleTxt");
    // Color + font come from container #configPanel stylesheet via #configTitleTxt selector
    configTitle->setAlignment(Qt::AlignCenter);
    configLayout->addWidget(configTitle);

    // Separator line
    QFrame* sepLine = new QFrame();
    sepLine->setFrameShape(QFrame::HLine);
    sepLine->setStyleSheet("QFrame { color: #e0e0e0; border: none; border-top: 1px solid #e0e0e0; }");
    configLayout->addWidget(sepLine);

    // Output format label
    QLabel* formatLabel = new QLabel(tr("输出格式"));
    formatLabel->setObjectName("configFormatLabel");
    // Color + font come from container #configPanel stylesheet via #configFormatLabel selector
    configLayout->addWidget(formatLabel);

    // Format combo — solid background explicitly
    m_formatCombo = new QComboBox();
    m_formatCombo->setMinimumHeight(28);
    m_formatCombo->setStyleSheet(
        "QComboBox { padding: 3px 8px; border: 2px solid #1976D2; border-radius: 5px; "
        "background-color: #ffffff; color: #333; font-size: 12px; min-width: 100px; }"
        "QComboBox::drop-down { border: none; width: 22px; "
        "background-color: #ffffff; }"
        "QComboBox::down-arrow { width: 10px; height: 10px; }"
        "QComboBox QAbstractItemView { "
        "border: 1px solid #ccc; border-radius: 4px; background-color: #ffffff; "
        "color: #333; selection-background-color: #e3f2fd; selection-color: #000; "
        "font-size: 12px; }"
    );
    configLayout->addWidget(m_formatCombo);

    // Output directory label
    QLabel* dirLabel = new QLabel(tr("输出目录"));
    dirLabel->setObjectName("configDirLabel");
    // Color + font come from container #configPanel stylesheet via #configDirLabel selector
    configLayout->addWidget(dirLabel);

    // Output directory input + browse button
    QHBoxLayout* dirRow = new QHBoxLayout();
    dirRow->setSpacing(5);

    m_outputDirEdit = new QLineEdit();
    m_outputDirEdit->setPlaceholderText(tr("留空则使用源文件所在目录"));
    m_outputDirEdit->setMinimumHeight(28);
    m_outputDirEdit->setStyleSheet(
        "QLineEdit { padding: 3px 8px; border: 1px solid #bbb; border-radius: 5px; "
        "background-color: #ffffff; color: #333; font-size: 12px; }"
    );
    dirRow->addWidget(m_outputDirEdit, 1);

    QPushButton* browseBtn = new QPushButton(tr("浏览"));
    browseBtn->setMinimumHeight(28);
    browseBtn->setFixedWidth(50);
    browseBtn->setStyleSheet(
        "QPushButton { padding: 3px 8px; border: 1px solid #bbb; border-radius: 5px; "
        "background-color: #f0f0f0; color: #333; font-size: 12px; }"
        "QPushButton:hover { background-color: #e0e0e0; }"
    );
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this,
            tr("选择输出目录"), m_outputDirEdit->text());
        if (!dir.isEmpty()) {
            m_outputDirEdit->setText(dir);
        }
    });
    dirRow->addWidget(browseBtn);

    configLayout->addLayout(dirRow);

    // Spacer
    configLayout->addStretch(1);

    // ── Parameter settings button ─────────────────────────────────
    m_paramsBtn = new QPushButton(style()->standardIcon(QStyle::SP_FileDialogDetailedView),
        tr(" 参数设置"));
    m_paramsBtn->setMinimumHeight(28);
    m_paramsBtn->setCursor(Qt::PointingHandCursor);
    m_paramsBtn->setStyleSheet(
        "QPushButton { border: 1px solid #64B5F6; border-radius: 5px; "
        "background-color: #E3F2FD; color: #1565C0; font-size: 12px; font-weight: bold; }"
        "QPushButton:hover { background-color: #BBDEFB; }"
        "QPushButton:pressed { background-color: #90CAF9; }"
    );
    m_paramsBtn->setIconSize(QSize(14, 14));
    configLayout->addWidget(m_paramsBtn);

    // Convert button
    m_convertBtn = new QPushButton(style()->standardIcon(QStyle::SP_MediaPlay),
        tr(" 开始转换"));
    m_convertBtn->setMinimumHeight(30);
    m_convertBtn->setCursor(Qt::PointingHandCursor);
    m_convertBtn->setStyleSheet(
        "QPushButton { border: none; border-radius: 5px; "
        "background-color: #1976D2; color: white; font-size: 13px; font-weight: bold; }"
        "QPushButton:hover { background-color: #1565C0; }"
        "QPushButton:pressed { background-color: #0D47A1; }"
        "QPushButton:disabled { background-color: #ccc; color: #888; }"
    );
    m_convertBtn->setIconSize(QSize(14, 14));
    configLayout->addWidget(m_convertBtn);

    contentLayout->addWidget(m_configPanel);

    mainLayout->addLayout(contentLayout, 2);

    // ── Bottom: progress + task list ──────────────────────────────
    QGroupBox* taskGroup = new QGroupBox(tr("任务列表"));
    taskGroup->setStyleSheet(
        "QGroupBox { font-weight: bold; border: 1px solid #ccc; border-radius: 4px; "
        "margin-top: 8px; padding-top: 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }"
    );
    QVBoxLayout* taskLayout = new QVBoxLayout(taskGroup);
    m_progressWidget = new ProgressWidget();
    taskLayout->addWidget(m_progressWidget);
    m_taskListWidget = new TaskListWidget();
    taskLayout->addWidget(m_taskListWidget, 1);
    mainLayout->addWidget(taskGroup, 1);

    setCentralWidget(centralWidget);

    // Initialise format combo for default (first) tab
    populateFormatCombo(Cat::Image);
}

void MainWindow::setupConnections() {
    TaskManager* tm = TaskManager::instance();
    connect(tm, &TaskManager::taskAdded, this, &MainWindow::onTaskAdded);
    connect(tm, &TaskManager::taskStarted, this, &MainWindow::onTaskStarted);
    connect(tm, &TaskManager::taskProgressChanged, this, &MainWindow::onTaskProgressChanged);
    connect(tm, &TaskManager::taskCompleted, this, &MainWindow::onTaskCompleted);
    connect(tm, &TaskManager::allTasksCompleted, this, &MainWindow::onAllTasksCompleted);

    // Tab change → update external config panel
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);

    // Convert button → start conversion
    connect(m_convertBtn, &QPushButton::clicked, this, &MainWindow::onStartConversion);

    // Parameter settings button → open dialog
    connect(m_paramsBtn, &QPushButton::clicked, this, &MainWindow::onConversionParams);

    // When files are added to the current tab, auto-populate output dir
    auto updateDirOnAdd = [this](FileCategoryWidget* tab) {
        if (m_tabWidget->currentWidget() == tab && tab->fileCount() > 0) {
            QFileInfo fi(tab->allFiles().first().filePath);
            m_outputDirEdit->setText(fi.absolutePath());
        }
    };
    connect(m_imageTab, &FileCategoryWidget::filesChanged, this,
        [this, updateDirOnAdd]() { updateDirOnAdd(m_imageTab); });
    connect(m_docTab, &FileCategoryWidget::filesChanged, this,
        [this, updateDirOnAdd]() { updateDirOnAdd(m_docTab); });
    connect(m_audioTab, &FileCategoryWidget::filesChanged, this,
        [this, updateDirOnAdd]() { updateDirOnAdd(m_audioTab); });
    connect(m_videoTab, &FileCategoryWidget::filesChanged, this,
        [this, updateDirOnAdd]() { updateDirOnAdd(m_videoTab); });
}

void MainWindow::populateFormatCombo(FormatRegistry::Category cat) {
    m_formatCombo->blockSignals(true);
    m_formatCombo->clear();

    const auto& reg = FormatRegistry::instance();
    QStringList formats;
    switch (cat) {
        case FormatRegistry::Category::Image:
            formats = reg.imageOutputFormats();
            break;
        case FormatRegistry::Category::Document:
            formats = reg.documentOutputFormats();
            break;
        case FormatRegistry::Category::Audio:
            formats = reg.audioFormats();
            break;
        case FormatRegistry::Category::Video:
            formats = reg.videoFormats();
            break;
        default:
            formats = reg.allFormats();
            break;
    }

    for (const QString& fmt : formats) {
        m_formatCombo->addItem(fmt.toUpper(), fmt.toLower());
    }

    // Restore saved selection if any
    if (m_savedFormats.contains(cat)) {
        QVariant saved = m_savedFormats.value(cat);
        int idx = m_formatCombo->findData(saved);
        if (idx >= 0)
            m_formatCombo->setCurrentIndex(idx);
    }

    m_formatCombo->blockSignals(false);
}

void MainWindow::onTabChanged(int index) {
    // Save current format for last active category (before switching)
    if (m_formatCombo->count() > 0 && m_formatCombo->currentIndex() >= 0) {
        m_savedFormats[m_lastActiveCategory] = m_formatCombo->currentData();
    }

    // Determine new category from the new index
    FormatRegistry::Category newCat = FormatRegistry::Category::Image;
    switch (index) {
        case 0: newCat = FormatRegistry::Category::Image; break;
        case 1: newCat = FormatRegistry::Category::Document; break;
        case 2: newCat = FormatRegistry::Category::Audio; break;
        case 3: newCat = FormatRegistry::Category::Video; break;
        default: break;
    }

    // Populate format combo for the new tab
    populateFormatCombo(newCat);

    // Update output dir from the new tab's first file
    FileCategoryWidget* tab = nullptr;
    switch (index) {
        case 0: tab = m_imageTab; break;
        case 1: tab = m_docTab; break;
        case 2: tab = m_audioTab; break;
        case 3: tab = m_videoTab; break;
        default: break;
    }
    if (tab && tab->fileCount() > 0 && !tab->allFiles().isEmpty()) {
        QFileInfo fi(tab->allFiles().first().filePath);
        m_outputDirEdit->setText(fi.absolutePath());
    }

    m_lastActiveCategory = newCat;
}

void MainWindow::onConversionParams() {
    // Determine current category from active tab
    FormatRegistry::Category currentCat = FormatRegistry::Category::Image;
    int idx = m_tabWidget->currentIndex();
    switch (idx) {
        case 0: currentCat = FormatRegistry::Category::Image; break;
        case 1: currentCat = FormatRegistry::Category::Document; break;
        case 2: currentCat = FormatRegistry::Category::Audio; break;
        case 3: currentCat = FormatRegistry::Category::Video; break;
        default: break;
    }

    // Reset button style before opening dialog
    m_paramsBtn->setStyleSheet(
        "QPushButton { border: 1px solid #64B5F6; border-radius: 6px; "
        "background-color: #E3F2FD; color: #1565C0; font-size: 13px; font-weight: bold; }"
        "QPushButton:hover { background-color: #BBDEFB; }"
        "QPushButton:pressed { background-color: #90CAF9; }"
    );

    ConversionParamsDialog dialog(this);
    dialog.setDarkMode(m_darkMode);
    dialog.setActiveCategory(currentCat);

    // Restore previously saved params for each category
    auto restoreCat = [&](FormatRegistry::Category cat) {
        if (m_conversionParams.contains(cat)) {
            dialog.setParamsForCategory(cat, m_conversionParams[cat]);
        }
    };
    restoreCat(FormatRegistry::Category::Image);
    restoreCat(FormatRegistry::Category::Document);
    restoreCat(FormatRegistry::Category::Audio);
    restoreCat(FormatRegistry::Category::Video);

    if (dialog.exec() == QDialog::Accepted) {
        // Store params from dialog for all categories
        m_conversionParams[FormatRegistry::Category::Image] =
            dialog.getParamsForCategory(FormatRegistry::Category::Image);
        m_conversionParams[FormatRegistry::Category::Document] =
            dialog.getParamsForCategory(FormatRegistry::Category::Document);
        m_conversionParams[FormatRegistry::Category::Audio] =
            dialog.getParamsForCategory(FormatRegistry::Category::Audio);
        m_conversionParams[FormatRegistry::Category::Video] =
            dialog.getParamsForCategory(FormatRegistry::Category::Video);

        LOG_INFO("MainWindow", "转换参数设置已更新");

        // Visual feedback
        m_paramsBtn->setStyleSheet(
            "QPushButton { border: 2px solid #4CAF50; border-radius: 6px; "
            "background-color: #E8F5E9; color: #2E7D32; font-size: 13px; font-weight: bold; }"
            "QPushButton:hover { background-color: #C8E6C9; }"
        );
        m_statusLabel->setText(tr("转换参数已设置"));
    }
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
    // Re-apply config panel-specific styles that the global theme might override
    m_configPanel->setStyleSheet(
        "#configPanel { border: 1px solid #bbb; border-radius: 8px; "
        "background-color: #ffffff; }"
        "#configPanel QLabel { color: #333; background: transparent; border: none; }"
        "#configTitleTxt { color: #1565C0; font-size: 16px; font-weight: bold; }"
        "#configFormatLabel, #configDirLabel { font-size: 13px; font-weight: bold; }"
    );
    // Reset params button to light style unless it was customized (green for accepted)
    QString currentBtnStyle = m_paramsBtn->styleSheet();
    if (!currentBtnStyle.contains("#4CAF50")) {
        m_paramsBtn->setStyleSheet(
            "QPushButton { border: 1px solid #64B5F6; border-radius: 6px; "
            "background-color: #E3F2FD; color: #1565C0; font-size: 13px; font-weight: bold; }"
            "QPushButton:hover { background-color: #BBDEFB; }"
            "QPushButton:pressed { background-color: #90CAF9; }"
        );
    }
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
        QGroupBox { font-weight: bold; border: 1px solid #3c3c3c; border-radius: 4px; margin-top: 8px; padding-top: 8px; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }
        QTableWidget { border: 1px solid #3c3c3c; border-radius: 4px; gridline-color: #3c3c3c; background: #252526; }
        QTableWidget::item:selected { background-color: #094771; color: #fff; }
        QHeaderView::section { background-color: #2d2d2d; border: none; border-bottom: 1px solid #3c3c3c; padding: 4px; font-weight: bold; }
        QPushButton { padding: 6px 12px; border: 1px solid #3c3c3c; border-radius: 4px; background-color: #333; color: #e0e0e0; }
        QPushButton:hover { background-color: #094771; }
        QPushButton:disabled { background-color: #2d2d2d; color: #666; }
        QCheckBox { spacing: 6px; color: #e0e0e0; }
        QLabel { color: #e0e0e0; }
        QScrollBar:vertical { background: #2d2d2d; width: 12px; }
        QScrollBar::handle:vertical { background: #555; border-radius: 6px; min-height: 30px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
    )";
    setStyleSheet(style);
    m_configPanel->setStyleSheet(
        "#configPanel { border: 1px solid #555; border-radius: 8px; "
        "background-color: #252526; }"
        "#configPanel QLabel { color: #e0e0e0; background: transparent; border: none; }"
        "#configTitleTxt { color: #64B5F6; font-size: 16px; font-weight: bold; }"
        "#configFormatLabel, #configDirLabel { font-size: 13px; font-weight: bold; }"
    );
    // Reset params button to dark style unless it was customized (green for accepted)
    QString currentBtnStyle = m_paramsBtn->styleSheet();
    if (!currentBtnStyle.contains("#4CAF50")) {
        m_paramsBtn->setStyleSheet(
            "QPushButton { border: 1px solid #1565C0; border-radius: 6px; "
            "background-color: #0D47A1; color: #90CAF9; font-size: 13px; font-weight: bold; }"
            "QPushButton:hover { background-color: #1565C0; }"
            "QPushButton:pressed { background-color: #1976D2; }"
        );
    }
}

// ── File handling ────────────────────────────────────────────────

void MainWindow::addFilesAndAutoRoute(const QStringList& filePaths) {
    if (filePaths.isEmpty()) return;

    int imageCount = m_imageTab->addFiles(filePaths);
    int docCount   = m_docTab->addFiles(filePaths);
    int audioCount = m_audioTab->addFiles(filePaths);
    int videoCount = m_videoTab->addFiles(filePaths);

    int total = imageCount + docCount + audioCount + videoCount;
    int skipped = filePaths.size() - total;

    QStringList parts;
    if (imageCount > 0) parts << tr("%1 个图片").arg(imageCount);
    if (docCount > 0)   parts << tr("%1 个文档").arg(docCount);
    if (audioCount > 0) parts << tr("%1 个音频").arg(audioCount);
    if (videoCount > 0) parts << tr("%1 个视频").arg(videoCount);

    QString msg;
    if (total > 0) {
        msg = tr("已添加 ") + parts.join("，");
        int maxCount = qMax(qMax(imageCount, docCount), qMax(audioCount, videoCount));
        if (maxCount == imageCount) m_tabWidget->setCurrentIndex(0);
        else if (maxCount == docCount) m_tabWidget->setCurrentIndex(1);
        else if (maxCount == audioCount) m_tabWidget->setCurrentIndex(2);
        else if (maxCount == videoCount) m_tabWidget->setCurrentIndex(3);
    }
    if (skipped > 0) {
        if (!msg.isEmpty()) msg += "；";
        msg += tr("%1 个文件格式不受支持（已跳过）").arg(skipped);
    }
    if (!msg.isEmpty()) {
        m_statusLabel->setText(msg);
    }
}

void MainWindow::onAddFiles() {
    const auto& reg = FormatRegistry::instance();
    QStringList files = QFileDialog::getOpenFileNames(this, tr("选择文件（自动识别分类）"),
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
        addFilesAndAutoRoute(files);
    }
}

void MainWindow::onStartConversion() {
    int totalFiles = m_imageTab->fileCount() + m_docTab->fileCount()
                   + m_audioTab->fileCount() + m_videoTab->fileCount();
    if (totalFiles == 0) {
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
        tr("<h3>集成格式转换工具 v1.3.1</h3>"
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
        auto c = TaskManager::instance()->counters();
        QString msg = tr("转换完成！\n成功: %1\n失败: %2").arg(c.completed).arg(c.failed);
        if (c.failed > 0) {
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

void MainWindow::onShowSummary() {
    showConversionSummary();
}

void MainWindow::onRetryFailed(const QList<QString>& inputPaths) {
    if (inputPaths.isEmpty()) return;
    addFilesAndAutoRoute(inputPaths);
    m_conversionResults.clear();
    m_startAction->setEnabled(false);
    onStartConversion();
}

void MainWindow::updateStatusBar() {
    TaskManager* tm = TaskManager::instance();
    auto c = tm->counters();
    m_taskStatsLabel->setText(
        tr("总计: %1 | 运行: %2 | 等待: %3 | 完成: %4 | 失败: %5")
        .arg(c.total).arg(c.running).arg(c.pending).arg(c.completed).arg(c.failed)
    );
}

void MainWindow::updateProgressWidget() {
    TaskManager* tm = TaskManager::instance();
    auto c = tm->counters();
    m_progressWidget->updateFromTaskManager(
        c.total, c.pending, c.running, c.completed, c.failed
    );
}

/// Convert bitrate string like "500k", "1M" to int kbps. Returns 0 if auto/empty.
static int parseBitrateToKbps(const QString& bitrateStr) {
    if (bitrateStr.isEmpty()) return 0;
    QString str = bitrateStr.trimmed().toLower();
    if (str == "auto") return 0;
    if (str.endsWith("k")) {
        bool ok;
        int val = str.left(str.length() - 1).toInt(&ok);
        return ok ? val : 0;
    }
    if (str.endsWith("m")) {
        bool ok;
        int val = str.left(str.length() - 1).toInt(&ok);
        return ok ? val * 1000 : 0;
    }
    bool ok;
    int val = str.toInt(&ok);
    return ok ? val : 0;
}

/// Merge saved conversion params into the task param map, normalizing keys for the converter.
static QVariantMap mergeConversionParams(const QVariantMap& baseParams,
                                          const QVariantMap& dialogParams,
                                          FormatRegistry::Category cat) {
    QVariantMap merged = baseParams;

    for (auto it = dialogParams.begin(); it != dialogParams.end(); ++it) {
        const QString& key = it.key();
        const QVariant& value = it.value();

        // Skip empty/default values to avoid overwriting base params
        if (!value.isValid()) continue;

        if (cat == FormatRegistry::Category::Audio || cat == FormatRegistry::Category::Video) {
            // Normalize resolution to lowercase (FFmpeg requires lowercase 'x')
            if (key == "resolution") {
                QString res = value.toString().trimmed().toLower();
                if (!res.isEmpty()) {
                    merged["resolution"] = res;
                }
                continue;
            }
            if (key == "videoBitrate" || key == "audioBitrate") {
                QString bs = value.toString();
                if (bs.isEmpty()) continue;
                int kbps = parseBitrateToKbps(bs);
                if (kbps > 0) {
                    merged[key] = kbps;
                }
                continue;
            }
            if (key == "framerate") {
                QString fps = value.toString();
                if (!fps.isEmpty()) {
                    bool ok;
                    int fpsInt = fps.toInt(&ok);
                    if (ok) {
                        merged["frameRate"] = fpsInt;
                    }
                }
                continue;
            }
            if (key == "twoPass") {
                merged["twoPass"] = value;
                continue;
            }
        }

        if (cat == FormatRegistry::Category::Document) {
            if (key == "pageSize" || key == "orientation" || key == "marginTop" ||
                key == "marginBottom" || key == "marginLeft" || key == "marginRight") {
                // Collect geometry variables for pandoc
                continue; // Handled below
            }
        }

        // Pass through all other values
        merged[key] = value;
    }

    // Build pandoc geometry variables
    if (cat == FormatRegistry::Category::Document) {
        QStringList geoParts;
        QString pageSize = dialogParams.value("pageSize").toString();
        if (!pageSize.isEmpty() && pageSize != "custom") {
            geoParts << pageSize;
        }
        QString orientation = dialogParams.value("orientation").toString();
        if (orientation == "landscape") {
            geoParts << "landscape";
        }
        auto addMargin = [&](const QString& key, const QString& side) {
            double val = dialogParams.value(key, 1.0).toDouble();
            if (val > 0) {
                geoParts << QString("%1=%2in").arg(side).arg(val, 0, 'f', 1);
            }
        };
        addMargin("marginTop", "top");
        addMargin("marginBottom", "bottom");
        addMargin("marginLeft", "left");
        addMargin("marginRight", "right");

        if (!geoParts.isEmpty()) {
            QVariantMap varMap = merged.value("variableMap").toMap();
            varMap["geometry"] = geoParts.join(",");
            merged["variableMap"] = varMap;
        }

        // Number sections
        if (dialogParams.value("numberSections", false).toBool()) {
            QStringList extraArgs = merged.value("extraArgs").toStringList();
            extraArgs << "--number-sections";
            merged["extraArgs"] = extraArgs;
        }
    }

    return merged;
}

void MainWindow::submitConversionTasks() {
    const auto& reg = FormatRegistry::instance();
    QString outputDir = m_outputDirEdit->text();
    if (outputDir.isEmpty()) {
        outputDir = QDir::homePath();
    }
    QDir().mkpath(outputDir);

    // Helper lambda: submit tasks for one category using its saved format
    auto submitTab = [&](FileCategoryWidget* tab, FormatRegistry::Category cat) {
        QList<FileInfo> files = tab->allFiles();
        if (files.isEmpty()) return;

        // Use per-category saved format; fall back to current combo selection
        QString outputFormat;
        QVariant saved = m_savedFormats.value(cat);
        if (saved.isValid()) {
            outputFormat = saved.toString();
        } else {
            outputFormat = m_formatCombo->currentData().toString();
        }

        // Get saved conversion params for this category (from dialog)
        QVariantMap dialogParams = m_conversionParams.value(cat);

        for (const FileInfo& fileInfo : files) {
            QFileInfo fi(fileInfo.filePath);
            QString baseName = fi.completeBaseName();
            QString outputFile = outputDir + "/" + baseName + "." + outputFormat;

            // Avoid overwriting source
            if (QFileInfo(outputFile).absoluteFilePath() == QFileInfo(fileInfo.filePath).absoluteFilePath()) {
                outputFile = outputDir + "/" + baseName + "_converted." + outputFormat;
                LOG_WARNING("MainWindow", QString("输出路径与输入相同，自动重命名: %1").arg(outputFile));
            }

            QVariantMap params;
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

            // Merge dialog conversion params
            if (!dialogParams.isEmpty()) {
                params = mergeConversionParams(params, dialogParams, cat);
            }

            TaskManager::instance()->addTask(fileInfo.filePath, outputFile, params);
        }
    };

    submitTab(m_imageTab, FormatRegistry::Category::Image);
    submitTab(m_docTab,   FormatRegistry::Category::Document);
    submitTab(m_audioTab, FormatRegistry::Category::Audio);
    submitTab(m_videoTab, FormatRegistry::Category::Video);

    LOG_INFO("MainWindow", "已从所有分类标签提交转换任务");
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

// ---------------------------------------------------------------------------
// Drag-and-drop
// ---------------------------------------------------------------------------
void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    // Accept the drop only if the drag payload contains at least one local
    // file URL. Other MIME types (text, images, etc.) are ignored so we
    // don't show the "no entry" cursor for things we can't handle.
    if (event->mimeData()->hasUrls() &&
        std::any_of(event->mimeData()->urls().cbegin(),
                    event->mimeData()->urls().cend(),
                    [](const QUrl& u) { return u.isLocalFile(); })) {
        event->acceptProposedAction();
    }
}

void MainWindow::dragMoveEvent(QDragMoveEvent* event) {
    // Mirror dragEnterEvent — without this the cursor reverts to "no entry"
    // while the user moves the file around the window.
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    if (!event->mimeData()->hasUrls()) {
        return;
    }
    QStringList paths;
    for (const QUrl& url : event->mimeData()->urls()) {
        if (url.isLocalFile()) {
            const QString localPath = url.toLocalFile();
            QFileInfo info(localPath);
            // Drop a folder → expand to its immediate children. This matches
            // what most users expect (dragging a folder in should add the
            // folder's contents, not just one "path/to/folder" string).
            if (info.isDir()) {
                QDir dir(localPath);
                const QStringList entries = dir.entryList(QDir::Files);
                for (const QString& name : entries) {
                    paths << dir.absoluteFilePath(name);
                }
            } else if (info.isFile()) {
                paths << localPath;
            }
        }
    }
    if (paths.isEmpty()) {
        return;
    }
    event->acceptProposedAction();
    addFilesAndAutoRoute(paths);
    LOG_INFO("MainWindow",
             QString("Dropped %1 file(s) onto main window").arg(paths.size()));
}
