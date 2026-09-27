#include "main_window.h"

#include "batch_conversion_summary.h"
#include "config_manager.h"
#include "conversion_params_dialog.h"
#include "conversion_planner.h"
#include "error_types.h"
#include "file_category_widget.h"
#include "file_info.h"
#include "format_registry.h"
#include "logger.h"
#include "progress_widget.h"
#include "task_list_widget.h"
#include "task_manager.h"
#include "theme.h"

#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QSpinBox>
#include <QStyle>
#include <QStyleHints>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include <chrono>
#include <type_traits>

namespace
{
/// Qt 6.12 ships two incompatible signatures for this setter across
/// point releases/builds (int on the local D:/tools/6.12.0 tree,
/// std::chrono::milliseconds on the one aqt installs in CI). Probe
/// which one exists with a C++23 requires-clause so both compile.
template<typename H>
concept WakeUpDelayTakesInt = requires(H* h, int ms) { h->setToolTipWakeUpDelay(ms); };

template<typename H>
void setToolTipWakeUpDelayCompat(H* hints, int ms)
{
    if (!hints)
    {
        return;
    }
    if constexpr (WakeUpDelayTakesInt<H>)
    {
        hints->setToolTipWakeUpDelay(ms);
    }
    else
    {
        hints->setToolTipWakeUpDelay(std::chrono::milliseconds(ms));
    }
}
} // namespace

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    m_taskListWidget(nullptr),
    m_progressWidget(nullptr),
    m_statusLabel(nullptr),
    m_taskStatsLabel(nullptr),
    m_startAction(nullptr),
    m_cancelAction(nullptr),
    m_summaryAction(nullptr),
    m_toolbarStartAction(nullptr),
    m_lastActiveCategory(FormatRegistry::Category::Image)
{
    setWindowTitle(tr("集成格式转换工具 v%1").arg(QStringLiteral(APP_VERSION)));
    resize(1200, 800);
    setAcceptDrops(true); // Enable drag-and-drop of files onto the main window
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupCentralWidget();
    setupConnections();
    setNavSelected(0);

    // Qt 6.12: tooltip wake-up delay. The setter signature differs between
    // Qt 6.12 builds (plain int vs std::chrono::milliseconds); the template
    // + if constexpr probe below compiles on both because the untaken
    // branch is never instantiated.
    setToolTipWakeUpDelayCompat(QApplication::styleHints(), 500);

    LOG_INFO("MainWindow", "主窗口初始化完成 (外部配置面板)");
}

MainWindow::~MainWindow()
{ }

void MainWindow::setupMenuBar()
{
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
    QMenu* themeMenu = viewMenu->addMenu(tr("外观(&A)"));
    QActionGroup* themeGroup = new QActionGroup(this);
    themeGroup->setExclusive(true);
    for (auto [label, mode] :
         std::initializer_list<std::pair<const char*, Theme::Mode>>{{QT_TR_NOOP("跟随系统"), Theme::Mode::System},
                                                                    {QT_TR_NOOP("浅色"), Theme::Mode::Light},
                                                                    {QT_TR_NOOP("深色"), Theme::Mode::Dark}})
    {
        QAction* act = themeMenu->addAction(tr(label));
        act->setCheckable(true);
        act->setData(static_cast<int>(mode));
        themeGroup->addAction(act);
        connect(act, &QAction::triggered, this, [this, mode]() { setThemeMode(mode); });
        if (mode == Theme::loadMode())
            act->setChecked(true);
    }
    viewMenu->addSeparator();
    QAction* pauseAct = viewMenu->addAction(tr("暂停/恢复队列"));
    pauseAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
    connect(pauseAct, &QAction::triggered, this, &MainWindow::togglePauseQueue);

    QMenu* helpMenu = menuBar->addMenu(tr("帮助(&H)"));
    QAction* aboutAction = helpMenu->addAction(tr("关于(&A)"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::setupToolBar()
{
    QToolBar* toolBar = addToolBar(tr("主工具栏"));
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(24, 24));

    QAction* addFilesAction = toolBar->addAction(QIcon(":/icons/file.svg"), tr("添加文件"));
    connect(addFilesAction, &QAction::triggered, this, &MainWindow::onAddFiles);

    toolBar->addSeparator();

    m_toolbarStartAction = toolBar->addAction(QIcon(":/icons/play.svg"), tr("开始转换"));
    connect(m_toolbarStartAction, &QAction::triggered, this, &MainWindow::onStartConversion);
}

void MainWindow::setupStatusBar()
{
    QStatusBar* statusBar = this->statusBar();
    m_statusLabel = new QLabel(tr("就绪"));
    m_statusLabel->setStyleSheet("padding: 2px 8px;");
    m_taskStatsLabel = new QLabel();
    m_taskStatsLabel->setStyleSheet("padding: 2px 8px;");
    m_taskStatsLabel->setObjectName("queueCounters");
    statusBar->addWidget(m_statusLabel, 1);
    statusBar->addPermanentWidget(m_taskStatsLabel);
}

void MainWindow::setupCentralWidget()
{
    QWidget* centralWidget = new QWidget(this);
    QHBoxLayout* rootLayout = new QHBoxLayout(centralWidget);
    rootLayout->setContentsMargins(8, 8, 8, 8);
    rootLayout->setSpacing(8);

    // ── Sidebar: search + functional navigation (3FUI pattern) ────
    m_sidebar = new QFrame();
    m_sidebar->setObjectName("sidebar");
    m_sidebar->setFixedWidth(200);
    QVBoxLayout* sideLayout = new QVBoxLayout(m_sidebar);
    sideLayout->setContentsMargins(8, 10, 8, 8);
    sideLayout->setSpacing(2);

    m_sidebarSearch = new QLineEdit();
    m_sidebarSearch->setPlaceholderText(tr("搜索功能…"));
    m_sidebarSearch->setClearButtonEnabled(true);
    sideLayout->addWidget(m_sidebarSearch);

    struct NavItem
    {
        const char* label;
    };
    const NavItem navItems[] = {{QT_TR_NOOP("转换队列")}, {QT_TR_NOOP("图片转换")}, {QT_TR_NOOP("文档转换")},
                                {QT_TR_NOOP("音频转换")}, {QT_TR_NOOP("视频转换")}, {QT_TR_NOOP("设置")}};
    QLabel* navSection = new QLabel(tr("功能导航"));
    Theme::setCss(navSection, "nav-section");
    sideLayout->addWidget(navSection);
    for (int i = 0; i < 6; ++i)
    {
        QPushButton* nav = new QPushButton(tr(navItems[i].label));
        nav->setCheckable(true);
        nav->setCursor(Qt::PointingHandCursor);
        nav->setProperty("cssClass", "nav");
        connect(nav, &QPushButton::clicked, this, [this, i]() { onSidebarNav(i); });
        m_navButtons.append(nav);
        sideLayout->addWidget(nav);
    }
    sideLayout->addStretch();
    rootLayout->addWidget(m_sidebar);

    // ── Right column ──────────────────────────────────────────────
    QVBoxLayout* rightCol = new QVBoxLayout();
    rightCol->setSpacing(8);
    rootLayout->addLayout(rightCol, 1);

    // Global action bar
    QHBoxLayout* globalBar = new QHBoxLayout();
    QPushButton* globalAddBtn = new QPushButton(QIcon(":/icons/file.svg"), tr(" 添加文件"));
    Theme::setCss(globalAddBtn, "primary");
    connect(globalAddBtn, &QPushButton::clicked, this, &MainWindow::onAddFiles);
    QLabel* globalHint = new QLabel(tr("支持拖拽 · 图片 / 文档 / 音频 / 视频自动分类"));
    Theme::setCss(globalHint, "muted");
    globalBar->addWidget(globalAddBtn);
    globalBar->addWidget(globalHint, 1);
    rightCol->addLayout(globalBar);

    // ── Content row: page stack + config panel ────────────────────
    QHBoxLayout* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(8);
    rightCol->addLayout(contentLayout, 1);

    m_pageStack = new QStackedWidget();
    m_pageStack->setMinimumWidth(0);
    m_pageStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    contentLayout->addWidget(m_pageStack, 1);

    // Page 0: conversion queue — semantic action toolbar (3FUI colors)
    m_queuePage = new QWidget();
    QVBoxLayout* queueLayout = new QVBoxLayout(m_queuePage);
    queueLayout->setContentsMargins(8, 8, 8, 8);
    queueLayout->setSpacing(8);

    QHBoxLayout* queueBar = new QHBoxLayout();
    queueBar->setSpacing(4);
    m_queuePauseBtn = new QPushButton(tr("暂停"));
    Theme::setCss(m_queuePauseBtn, "text-warning");
    m_queuePauseBtn->setToolTip(tr("暂停派发新任务（运行中的任务不受影响）  Ctrl+P"));
    m_queuePauseBtn->setEnabled(false);
    queueBar->addWidget(m_queuePauseBtn);
    m_queueCancelBtn = new QPushButton(tr("全部停止"));
    Theme::setCss(m_queueCancelBtn, "text-danger");
    queueBar->addWidget(m_queueCancelBtn);
    m_queueRemoveBtn = new QPushButton(tr("移除"));
    Theme::setCss(m_queueRemoveBtn, "text-remove");
    m_queueRemoveBtn->setEnabled(false);
    queueBar->addWidget(m_queueRemoveBtn);
    m_queueRetryBtn = new QPushButton(tr("重试失败"));
    Theme::setCss(m_queueRetryBtn, "text-locate");
    m_queueRetryBtn->setEnabled(false);
    queueBar->addWidget(m_queueRetryBtn);
    m_queueOpenBtn = new QPushButton(tr("打开输出目录"));
    Theme::setCss(m_queueOpenBtn, "text-success");
    m_queueOpenBtn->setEnabled(false);
    queueBar->addWidget(m_queueOpenBtn);
    queueBar->addStretch();
    queueLayout->addLayout(queueBar);

    m_progressWidget = new ProgressWidget();
    m_progressWidget->setMinimumWidth(0);
    m_progressWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    queueLayout->addWidget(m_progressWidget);
    m_taskListWidget = new TaskListWidget();
    queueLayout->addWidget(m_taskListWidget, 1);
    m_queueHint = new QLabel(tr("暂无任务 — 点击上方“添加文件”，或直接把文件拖进窗口"));
    QLabel* queueHint = m_queueHint;
    Theme::setCss(queueHint, "muted");
    queueHint->setAlignment(Qt::AlignCenter);
    queueLayout->addWidget(queueHint);
    m_pageStack->addWidget(m_queuePage);

    // Pages 1..4: category file lists
    using Cat = FormatRegistry::Category;
    m_imageTab = new FileCategoryWidget(Cat::Image);
    m_docTab = new FileCategoryWidget(Cat::Document);
    m_audioTab = new FileCategoryWidget(Cat::Audio);
    m_videoTab = new FileCategoryWidget(Cat::Video);
    m_pageStack->addWidget(m_imageTab);
    m_pageStack->addWidget(m_docTab);
    m_pageStack->addWidget(m_audioTab);
    m_pageStack->addWidget(m_videoTab);

    // Page 5: settings
    setupSettingsPage();
    m_pageStack->addWidget(m_settingsPage);

    // ── Config panel (right, sticky across pages) ─────────────────
    m_configPanel = new QFrame();
    m_configPanel->setObjectName("configPanel");
    m_configPanel->setFixedWidth(280);
    Theme::setCss(m_configPanel, "panel");

    QVBoxLayout* configLayout = new QVBoxLayout(m_configPanel);
    configLayout->setContentsMargins(14, 14, 14, 12);
    configLayout->setSpacing(8);

    QLabel* configTitle = new QLabel(tr("转换设置"));
    configTitle->setObjectName("configTitleTxt");
    Theme::setCss(configTitle, "section");
    configTitle->setAlignment(Qt::AlignCenter);
    configLayout->addWidget(configTitle);

    QLabel* formatLabel = new QLabel(tr("输出格式"));
    formatLabel->setObjectName("configFormatLabel");
    Theme::setCss(formatLabel, "muted");
    configLayout->addWidget(formatLabel);

    m_formatCombo = new QComboBox();
    m_formatCombo->setMinimumHeight(30);
    configLayout->addWidget(m_formatCombo);

    QLabel* dirLabel = new QLabel(tr("输出目录"));
    dirLabel->setObjectName("configDirLabel");
    Theme::setCss(dirLabel, "muted");
    configLayout->addWidget(dirLabel);

    QHBoxLayout* dirRow = new QHBoxLayout();
    dirRow->setSpacing(5);
    m_outputDirEdit = new QLineEdit();
    m_outputDirEdit->setPlaceholderText(tr("留空则使用源文件所在目录"));
    m_outputDirEdit->setMinimumHeight(30);
    dirRow->addWidget(m_outputDirEdit, 1);
    QPushButton* browseBtn = new QPushButton(tr("浏览"));
    browseBtn->setMinimumHeight(30);
    browseBtn->setFixedWidth(52);
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr("选择输出目录"), m_outputDirEdit->text());
        if (!dir.isEmpty())
        {
            m_outputDirEdit->setText(dir);
        }
    });
    dirRow->addWidget(browseBtn);
    configLayout->addLayout(dirRow);

    configLayout->addSpacing(6);

    m_paramsBtn = new QPushButton(QIcon(":/icons/settings.svg"), tr(" 参数设置"));
    m_paramsBtn->setMinimumHeight(32);
    m_paramsBtn->setCursor(Qt::PointingHandCursor);
    Theme::setCss(m_paramsBtn, "accent-outline");
    m_paramsBtn->setIconSize(QSize(14, 14));
    configLayout->addWidget(m_paramsBtn);

    m_convertBtn = new QPushButton(QIcon(":/icons/play.svg"), tr(" 开始转换"));
    m_convertBtn->setMinimumHeight(38);
    m_convertBtn->setCursor(Qt::PointingHandCursor);
    Theme::setCss(m_convertBtn, "primary");
    m_convertBtn->setIconSize(QSize(16, 16));
    configLayout->addWidget(m_convertBtn);

    // Top-aligned so the card hugs its content instead of stretching to the
    // page height and growing a hollow middle.
    contentLayout->addWidget(m_configPanel, 0, Qt::AlignTop);

    setCentralWidget(centralWidget);

    // Initialise format combo for default category
    populateFormatCombo(Cat::Image);
}

void MainWindow::setupSettingsPage()
{
    m_settingsPage = new QWidget();
    QVBoxLayout* pageLayout = new QVBoxLayout(m_settingsPage);
    pageLayout->setContentsMargins(8, 8, 8, 8);
    pageLayout->setSpacing(10);

    QLabel* title = new QLabel(tr("软件设置"));
    Theme::setCss(title, "title");
    pageLayout->addWidget(title);

    // Appearance
    QGroupBox* appearanceGroup = new QGroupBox(tr("外观"));
    QHBoxLayout* appearanceRow = new QHBoxLayout(appearanceGroup);
    QLabel* themeHint = new QLabel(tr("界面主题（即时生效，保存于配置）"));
    Theme::setCss(themeHint, "muted");
    appearanceRow->addWidget(themeHint);
    appearanceRow->addStretch();
    for (auto [label, mode] :
         std::initializer_list<std::pair<const char*, Theme::Mode>>{{QT_TR_NOOP("跟随系统"), Theme::Mode::System},
                                                                    {QT_TR_NOOP("浅色"), Theme::Mode::Light},
                                                                    {QT_TR_NOOP("深色"), Theme::Mode::Dark}})
    {
        QPushButton* b = new QPushButton(tr(label));
        b->setCheckable(true);
        b->setProperty("mode", static_cast<int>(mode));
        if (mode == Theme::loadMode())
            b->setChecked(true);
        connect(b, &QPushButton::clicked, this, [this, mode]() { setThemeMode(mode); });
        appearanceRow->addWidget(b);
    }
    pageLayout->addWidget(appearanceGroup);

    // Performance
    QGroupBox* perfGroup = new QGroupBox(tr("性能"));
    QGridLayout* perfRow = new QGridLayout(perfGroup);
    perfRow->addWidget(new QLabel(tr("最大并行任务数")), 0, 0);
    QSpinBox* parallelSpin = new QSpinBox();
    parallelSpin->setRange(1, QThread::idealThreadCount() * 2);
    parallelSpin->setValue(ConfigManager::instance().maxParallelTasks());
    connect(parallelSpin, qOverload<int>(&QSpinBox::valueChanged), this, [](int v) {
        ConfigManager::instance().setMaxParallelTasks(v);
        TaskManager::instance()->setMaxParallelTasks(v);
    });
    perfRow->addWidget(parallelSpin, 0, 1);
    perfRow->addWidget(new QLabel(tr("日志级别")), 1, 0);
    QComboBox* logCombo = new QComboBox();
    logCombo->addItems({"Debug", "Info", "Warning", "Error"});
    logCombo->setCurrentIndex(ConfigManager::instance().logLevel());
    connect(logCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [](int idx) {
        ConfigManager::instance().setLogLevel(idx);
        if (Logger* lg = g_logger.load())
            lg->setLevel(static_cast<Logger::Level>(idx));
    });
    perfRow->addWidget(logCombo, 1, 1);
    perfRow->setColumnStretch(2, 1);
    pageLayout->addWidget(perfGroup);

    // Notifications
    QGroupBox* notifyGroup = new QGroupBox(tr("通知"));
    QVBoxLayout* notifyRow = new QVBoxLayout(notifyGroup);
    QCheckBox* notifyCheck = new QCheckBox(tr("所有任务完成后弹窗提示"));
    notifyCheck->setChecked(ConfigManager::instance().value("showNotification", true).toBool());
    connect(notifyCheck, &QCheckBox::toggled, this,
            [](bool on) { ConfigManager::instance().setValue("showNotification", on); });
    notifyRow->addWidget(notifyCheck);
    pageLayout->addWidget(notifyGroup);

    pageLayout->addStretch(1);
}

namespace
{
constexpr int kQueuePage = 0;
constexpr int kSettingsPage = 5;

int pageForCategory(FormatRegistry::Category cat)
{
    using C = FormatRegistry::Category;
    switch (cat)
    {
        case C::Image:
            return 1;
        case C::Document:
            return 2;
        case C::Audio:
            return 3;
        case C::Video:
            return 4;
        default:
            return 1;
    }
}

FormatRegistry::Category categoryForPage(int page)
{
    using C = FormatRegistry::Category;
    switch (page)
    {
        case 2:
            return C::Document;
        case 3:
            return C::Audio;
        case 4:
            return C::Video;
        default:
            return C::Image;
    }
}
} // namespace

void MainWindow::setupConnections()
{
    TaskManager* tm = TaskManager::instance();
    connect(tm, &TaskManager::taskAdded, this, &MainWindow::onTaskAdded);
    connect(tm, &TaskManager::taskStarted, this, &MainWindow::onTaskStarted);
    connect(tm, &TaskManager::taskProgressChanged, this, &MainWindow::onTaskProgressChanged);
    connect(tm, &TaskManager::taskCompleted, this, &MainWindow::onTaskCompleted);
    connect(tm, &TaskManager::allTasksCompleted, this, &MainWindow::onAllTasksCompleted);
    connect(tm, &TaskManager::pauseStateChanged, this, &MainWindow::onQueuePauseStateChanged);

    // Convert button → start conversion
    connect(m_convertBtn, &QPushButton::clicked, this, &MainWindow::onStartConversion);

    // Parameter settings button → open dialog
    connect(m_paramsBtn, &QPushButton::clicked, this, &MainWindow::onConversionParams);

    // Queue semantic actions (3FUI toolbar)
    connect(m_queuePauseBtn, &QPushButton::clicked, this, &MainWindow::togglePauseQueue);
    connect(m_queueCancelBtn, &QPushButton::clicked, this, &MainWindow::onCancelAll);
    connect(m_queueRemoveBtn, &QPushButton::clicked, this, [this]() { m_taskListWidget->removeSelected(); });
    connect(m_queueRetryBtn, &QPushButton::clicked, this, [this]() {
        QStringList failed;
        for (ConversionTask* t : TaskManager::instance()->getFailedTasks())
            failed << t->inputFile();
        if (!failed.isEmpty())
            onRetryFailed(failed);
    });
    connect(m_queueOpenBtn, &QPushButton::clicked, this, [this]() {
        const QString taskId = m_taskListWidget->selectedTaskId();
        if (taskId.isEmpty())
            return;
        if (ConversionTask* t = TaskManager::instance()->getTask(taskId))
        {
            QFileInfo fi(t->outputFile());
            QDesktopServices::openUrl(QUrl::fromLocalFile(fi.absolutePath()));
        }
    });
    connect(m_taskListWidget, &TaskListWidget::selectionChanged, this, [this](const QString& taskId) {
        const bool has = !taskId.isEmpty();
        m_queueRemoveBtn->setEnabled(has);
        m_queueOpenBtn->setEnabled(has);
    });
    // The empty-state hint only matters while the queue is literally empty.
    auto syncHint = [this]() {
        if (m_queueHint)
            m_queueHint->setVisible(TaskManager::instance()->totalTaskCount() == 0);
    };
    connect(TaskManager::instance(), &TaskManager::taskAdded, this, syncHint);
    connect(TaskManager::instance(), &TaskManager::taskRemoved, this, syncHint);
    connect(m_taskListWidget, &TaskListWidget::rowActivated, this, [this](const QString& taskId) {
        if (ConversionTask* t = TaskManager::instance()->getTask(taskId))
        {
            QFileInfo fi(t->outputFile());
            QDesktopServices::openUrl(QUrl::fromLocalFile(fi.absolutePath()));
        }
    });

    // Sidebar search filters nav buttons (3FUI: 搜索选项卡标题)
    connect(m_sidebarSearch, &QLineEdit::textChanged, this, &MainWindow::onSidebarFilterChanged);

    // When files are added to the current page's tab, auto-populate output dir
    auto updateDirOnAdd = [this](FileCategoryWidget* tab) {
        if (m_pageStack->currentWidget() == tab && tab->fileCount() > 0)
        {
            QFileInfo fi(tab->allFiles().first().filePath);
            m_outputDirEdit->setText(fi.absolutePath());
        }
    };
    connect(m_imageTab, &FileCategoryWidget::filesChanged, this,
            [this, updateDirOnAdd]() { updateDirOnAdd(m_imageTab); });
    connect(m_docTab, &FileCategoryWidget::filesChanged, this, [this, updateDirOnAdd]() { updateDirOnAdd(m_docTab); });
    connect(m_audioTab, &FileCategoryWidget::filesChanged, this,
            [this, updateDirOnAdd]() { updateDirOnAdd(m_audioTab); });
    connect(m_videoTab, &FileCategoryWidget::filesChanged, this,
            [this, updateDirOnAdd]() { updateDirOnAdd(m_videoTab); });
}

void MainWindow::populateFormatCombo(FormatRegistry::Category cat)
{
    m_formatCombo->blockSignals(true);
    m_formatCombo->clear();

    const auto& reg = FormatRegistry::instance();
    QStringList formats;
    switch (cat)
    {
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

    for (const QString& fmt : formats)
    {
        m_formatCombo->addItem(fmt.toUpper(), fmt.toLower());
    }

    // Restore saved selection if any
    if (m_savedFormats.contains(cat))
    {
        QVariant saved = m_savedFormats.value(cat);
        int idx = m_formatCombo->findData(saved);
        if (idx >= 0)
            m_formatCombo->setCurrentIndex(idx);
    }

    m_formatCombo->blockSignals(false);
}

void MainWindow::setNavSelected(int index)
{
    for (int i = 0; i < m_navButtons.size(); ++i)
    {
        m_navButtons[i]->setChecked(i == index);
        m_navButtons[i]->setProperty("selected", i == index ? "true" : "false");
    }
}

void MainWindow::onSidebarNav(int index)
{
    if (index < 0 || index >= m_pageStack->count())
        return;
    m_pageStack->setCurrentIndex(index);
    setNavSelected(index);

    // Config panel follows the category (queue/settings pages keep last cat).
    if (index >= 1 && index <= 4)
    {
        FormatRegistry::Category newCat = categoryForPage(index);
        if (newCat != m_lastActiveCategory)
        {
            if (m_formatCombo->count() > 0 && m_formatCombo->currentIndex() >= 0)
                m_savedFormats[m_lastActiveCategory] = m_formatCombo->currentData();
            populateFormatCombo(newCat);
            m_lastActiveCategory = newCat;
        }
        FileCategoryWidget* tab = qobject_cast<FileCategoryWidget*>(m_pageStack->currentWidget());
        if (tab && tab->fileCount() > 0 && !tab->allFiles().isEmpty())
        {
            QFileInfo fi(tab->allFiles().first().filePath);
            m_outputDirEdit->setText(fi.absolutePath());
        }
    }
    m_lastNavIndex = index;
}

void MainWindow::onSidebarFilterChanged(const QString& text)
{
    const QString needle = text.trimmed();
    for (QPushButton* nav : m_navButtons)
    {
        nav->setVisible(needle.isEmpty() || nav->text().contains(needle, Qt::CaseInsensitive));
    }
}

void MainWindow::onConversionParams()
{
    // Determine current category from active page (queue page → last cat)
    int idx = m_pageStack->currentIndex();
    FormatRegistry::Category currentCat = (idx >= 1 && idx <= 4) ? categoryForPage(idx) : m_lastActiveCategory;

    Theme::setCss(m_paramsBtn, "accent-outline");

    ConversionParamsDialog dialog(this);
    dialog.setDarkMode(Theme::currentMode() == Theme::Mode::Dark);
    dialog.setActiveCategory(currentCat);

    // Restore previously saved params for each category
    auto restoreCat = [&](FormatRegistry::Category cat) {
        if (m_conversionParams.contains(cat))
        {
            dialog.setParamsForCategory(cat, m_conversionParams[cat]);
        }
    };
    restoreCat(FormatRegistry::Category::Image);
    restoreCat(FormatRegistry::Category::Document);
    restoreCat(FormatRegistry::Category::Audio);
    restoreCat(FormatRegistry::Category::Video);

    if (dialog.exec() == QDialog::Accepted)
    {
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
        Theme::setCss(m_paramsBtn, "success-outline");
        m_statusLabel->setText(tr("转换参数已设置"));
    }
}

void MainWindow::setThemeMode(Theme::Mode mode)
{
    Theme::saveMode(mode);
    // Re-polish widgets whose property-based QSS was cached (Qt quirk:
    // unpolish/polish forces re-evaluation of attribute selectors).
    for (QWidget* w : findChildren<QWidget*>())
    {
        w->style()->unpolish(w);
        w->style()->polish(w);
    }
    LOG_INFO("MainWindow",
             QString("界面主题已切换: %1")
                 .arg(mode == Theme::Mode::Dark ? "dark" : (mode == Theme::Mode::Light ? "light" : "system")));
}

void MainWindow::togglePauseQueue()
{
    TaskManager* tm = TaskManager::instance();
    if (tm->isPaused())
        tm->resume();
    else
        tm->pause();
}

void MainWindow::onQueuePauseStateChanged(bool paused)
{
    m_queuePauseBtn->setText(paused ? tr("恢复") : tr("暂停"));
    m_statusLabel->setText(paused ? tr("队列已暂停") : tr("队列已恢复"));
}

// ── File handling ────────────────────────────────────────────────

void MainWindow::addFilesAndAutoRoute(const QStringList& filePaths)
{
    if (filePaths.isEmpty())
        return;

    int imageCount = m_imageTab->addFiles(filePaths);
    int docCount = m_docTab->addFiles(filePaths);
    int audioCount = m_audioTab->addFiles(filePaths);
    int videoCount = m_videoTab->addFiles(filePaths);

    int total = imageCount + docCount + audioCount + videoCount;
    int skipped = filePaths.size() - total;

    QStringList parts;
    if (imageCount > 0)
        parts << tr("%1 个图片").arg(imageCount);
    if (docCount > 0)
        parts << tr("%1 个文档").arg(docCount);
    if (audioCount > 0)
        parts << tr("%1 个音频").arg(audioCount);
    if (videoCount > 0)
        parts << tr("%1 个视频").arg(videoCount);

    QString msg;
    if (total > 0)
    {
        msg = tr("已添加 ") + parts.join("，");
        int maxCount = qMax(qMax(imageCount, docCount), qMax(audioCount, videoCount));
        int targetPage = 1;
        if (maxCount == imageCount)
            targetPage = 1;
        else if (maxCount == docCount)
            targetPage = 2;
        else if (maxCount == audioCount)
            targetPage = 3;
        else if (maxCount == videoCount)
            targetPage = 4;
        onSidebarNav(targetPage);
    }
    if (skipped > 0)
    {
        if (!msg.isEmpty())
            msg += "；";
        msg += tr("%1 个文件格式不受支持（已跳过）").arg(skipped);
    }
    if (!msg.isEmpty())
    {
        m_statusLabel->setText(msg);
    }
}

void MainWindow::onAddFiles()
{
    const auto& reg = FormatRegistry::instance();
    QStringList files = QFileDialog::getOpenFileNames(this, tr("选择文件（自动识别分类）"), QString(),
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
                                                          .arg(reg.fileDialogDocumentFilter()));
    if (!files.isEmpty())
    {
        addFilesAndAutoRoute(files);
    }
}

void MainWindow::onStartConversion()
{
    int totalFiles =
        m_imageTab->fileCount() + m_docTab->fileCount() + m_audioTab->fileCount() + m_videoTab->fileCount();
    if (totalFiles == 0)
    {
        QMessageBox::warning(this, tr("警告"), tr("请先添加要转换的文件"));
        return;
    }
    m_conversionResults.clear();
    submitConversionTasks();
    TaskManager::instance()->start();
    m_statusLabel->setText(tr("正在转换..."));
    if (m_queuePauseBtn)
        m_queuePauseBtn->setEnabled(true);
    m_startAction->setEnabled(false);
    m_summaryAction->setEnabled(false);
    m_toolbarStartAction->setEnabled(false);
    LOG_INFO("MainWindow", "开始转换任务");
}

void MainWindow::onCancelAll()
{
    QMessageBox::StandardButton reply =
        QMessageBox::question(this, tr("确认"), tr("确定要取消所有任务吗？"), QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes)
    {
        TaskManager::instance()->cancelAllTasks();
        m_statusLabel->setText(tr("已取消"));
        m_startAction->setEnabled(true);
        m_toolbarStartAction->setEnabled(true);
        LOG_INFO("MainWindow", "取消所有任务");
    }
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, tr("关于"),
                       tr("<h3>集成格式转换工具 v%1</h3>").arg(QStringLiteral(APP_VERSION)) +
                           "<p>基于FFmpeg、Pandoc和ImageMagick的多功能文件转换工具</p>"
                           "<p>支持功能：</p>"
                           "<ul>"
                           "<li>视频格式转换（MP4, AVI, MKV等）</li>"
                           "<li>音频格式转换（MP3, WAV, FLAC等）</li>"
                           "<li>图片格式转换（PNG, JPG, GIF, WebP等）</li>"
                           "<li>文档格式转换（Markdown, DOCX, PDF等）</li>"
                           "</ul>"
                           "<p>© 2024 ConverterTools</p>");
}

void MainWindow::onExit()
{
    close();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // Cancel all running tasks before the window is destroyed.
    // This kills child processes (FFmpeg/Pandoc/ImageMagick) so they don't
    // become orphans that outlive the application.
    TaskManager::instance()->cancelAllTasks();
    event->accept();
}

void MainWindow::onTaskAdded(const QString& taskId)
{
    Q_UNUSED(taskId);
    updateStatusBar();
    updateProgressWidget();
    m_taskListWidget->refreshTaskList();
}

void MainWindow::onTaskStarted(const QString& taskId)
{
    ConversionTask* task = TaskManager::instance()->getTask(taskId);
    if (task)
    {
        m_currentConvertingFile = task->inputFile();
        QFileInfo fi(m_currentConvertingFile);
        m_progressWidget->setCurrentFile(fi.fileName());
    }
    updateStatusBar();
    updateProgressWidget();
}

void MainWindow::onTaskProgressChanged(const QString& taskId, int progress)
{
    m_taskListWidget->updateTaskProgress(taskId, progress);
    updateProgressWidget();
}

void MainWindow::onTaskCompleted(const QString& taskId, bool success)
{
    ConversionTask* task = TaskManager::instance()->getTask(taskId);
    if (task)
    {
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

void MainWindow::onAllTasksCompleted()
{
    m_statusLabel->setText(tr("所有任务已完成"));
    m_startAction->setEnabled(true);
    if (m_queuePauseBtn)
        m_queuePauseBtn->setEnabled(false);
    m_summaryAction->setEnabled(!m_conversionResults.isEmpty());
    m_toolbarStartAction->setEnabled(true);
    m_progressWidget->setCurrentFile(QString());
    updateProgressWidget();
    m_taskListWidget->refreshTaskList();
    if (ConfigManager::instance().value("showNotification", true).toBool())
    {
        auto c = TaskManager::instance()->counters();
        QString msg = tr("转换完成！\n成功: %1\n失败: %2").arg(c.completed).arg(c.failed);
        if (c.failed > 0)
        {
            QMessageBox::warning(this, tr("转换完成"), msg);
        }
        else
        {
            QMessageBox::information(this, tr("转换完成"), msg);
        }
    }
    if (!m_conversionResults.isEmpty())
    {
        showConversionSummary();
    }
    LOG_INFO("MainWindow", "所有任务已完成");
}

void MainWindow::onShowSummary()
{
    showConversionSummary();
}

void MainWindow::onRetryFailed(const QList<QString>& inputPaths)
{
    if (inputPaths.isEmpty())
        return;
    addFilesAndAutoRoute(inputPaths);

    // §3.4 fix: resubmit ONLY the failed files. The old code called
    // onStartConversion(), which re-converted every file in every tab —
    // "retry 3 failures" silently re-encoded all previously-succeeded ones.
    const QSet<QString> retry(inputPaths.begin(), inputPaths.end());

    // Drop the stale failure records so the summary/results show the fresh
    // attempt instead of duplicates. Successes stay (they aren't re-run).
    for (auto it = m_conversionResults.begin(); it != m_conversionResults.end();)
    {
        if (retry.contains(it->inputPath) && !it->success)
        {
            it = m_conversionResults.erase(it);
        }
        else
        {
            ++it;
        }
    }

    m_startAction->setEnabled(false);
    m_summaryAction->setEnabled(false);
    submitConversionTasks(retry);
    TaskManager::instance()->start();
    m_statusLabel->setText(tr("正在重试失败任务..."));
    m_toolbarStartAction->setEnabled(false);
    LOG_INFO("MainWindow", QString("重试 %1 个失败任务").arg(retry.size()));
}

void MainWindow::updateStatusBar()
{
    TaskManager* tm = TaskManager::instance();
    auto c = tm->counters();
    const QString counters = tr("总计 %1 · 运行 %2 · 等待 %3 · 完成 %4 · 失败 %5")
                                 .arg(c.total)
                                 .arg(c.running)
                                 .arg(c.pending)
                                 .arg(c.completed)
                                 .arg(c.failed);
    m_taskStatsLabel->setText(counters);
    if (m_queuePauseBtn)
    {
        // Pause only makes sense while there is something left to dispatch;
        // keep it live while already paused so the user can resume.
        m_queuePauseBtn->setEnabled(c.pending > 0 || tm->isPaused());
    }
    if (m_queueRetryBtn)
        m_queueRetryBtn->setEnabled(c.failed > 0);
}

void MainWindow::updateProgressWidget()
{
    TaskManager* tm = TaskManager::instance();
    auto c = tm->counters();
    m_progressWidget->updateFromTaskManager(c.total, c.pending, c.running, c.completed, c.failed);
}

void MainWindow::submitConversionTasks(const QSet<QString>& onlyPaths)
{
    QString outputDir = m_outputDirEdit->text();
    if (outputDir.isEmpty())
    {
        outputDir = QDir::homePath();
    }
    QDir().mkpath(outputDir);

    // Helper lambda: submit tasks for one category using its saved format
    auto submitTab = [&](FileCategoryWidget* tab, FormatRegistry::Category cat) {
        QList<FileInfo> files = tab->allFiles();
        if (files.isEmpty())
            return;

        // Use per-category saved format; fall back to current combo selection
        QString outputFormat;
        QVariant saved = m_savedFormats.value(cat);
        if (saved.isValid())
        {
            outputFormat = saved.toString();
        }
        else
        {
            outputFormat = m_formatCombo->currentData().toString();
        }

        // Get saved conversion params for this category (from dialog)
        QVariantMap dialogParams = m_conversionParams.value(cat);

        for (const FileInfo& fileInfo : files)
        {
            // Retry mode: skip anything not in the requested path set.
            if (!onlyPaths.isEmpty() && !onlyPaths.contains(fileInfo.filePath))
            {
                continue;
            }
            // Output naming + converter routing now live in
            // ConversionPlanner (shared with the CLI; analysis §4.2).
            const QString outputFile =
                ConversionPlanner::outputPath(fileInfo.filePath, QString(), outputDir, outputFormat);
            if (QFileInfo(outputFile).fileName().contains("_converted."))
            {
                LOG_WARNING("MainWindow", QString("输出路径与输入相同，自动重命名: %1").arg(outputFile));
            }

            QVariantMap params;
            params["outputFormat"] = outputFormat;
            const QString converter =
                ConversionPlanner::converterNameFor(outputFormat, QFileInfo(fileInfo.filePath).suffix());
            params["converter"] = converter.isEmpty() ? QStringLiteral("FFmpeg") : converter;

            // Merge dialog conversion params
            if (!dialogParams.isEmpty())
            {
                params = ConversionPlanner::mergeParams(params, dialogParams, cat);
            }

            TaskManager::instance()->addTask(fileInfo.filePath, outputFile, params);
        }
    };

    submitTab(m_imageTab, FormatRegistry::Category::Image);
    submitTab(m_docTab, FormatRegistry::Category::Document);
    submitTab(m_audioTab, FormatRegistry::Category::Audio);
    submitTab(m_videoTab, FormatRegistry::Category::Video);

    LOG_INFO("MainWindow", "已从所有分类标签提交转换任务");
}

void MainWindow::showConversionSummary()
{
    BatchConversionSummary summary(this);
    summary.setResults(m_conversionResults);
    connect(&summary, &BatchConversionSummary::retryRequested, this, &MainWindow::onRetryFailed);
    summary.exec();
}

void MainWindow::onErrorOccurred(const ErrorInfo& error)
{
    Q_UNUSED(error);
}

void MainWindow::onRetryTriggered(const QString& taskId, int retryCount)
{
    Q_UNUSED(taskId);
    Q_UNUSED(retryCount);
}

// ---------------------------------------------------------------------------
// Drag-and-drop
// ---------------------------------------------------------------------------
void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    // Accept the drop only if the drag payload contains at least one local
    // file URL. Other MIME types (text, images, etc.) are ignored so we
    // don't show the "no entry" cursor for things we can't handle.
    if (event->mimeData()->hasUrls() &&
        std::any_of(event->mimeData()->urls().cbegin(), event->mimeData()->urls().cend(),
                    [](const QUrl& u) { return u.isLocalFile(); }))
    {
        event->acceptProposedAction();
    }
}

void MainWindow::dragMoveEvent(QDragMoveEvent* event)
{
    // Mirror dragEnterEvent — without this the cursor reverts to "no entry"
    // while the user moves the file around the window.
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event)
{
    if (!event->mimeData()->hasUrls())
    {
        return;
    }
    QStringList paths;
    for (const QUrl& url : event->mimeData()->urls())
    {
        if (url.isLocalFile())
        {
            const QString localPath = url.toLocalFile();
            QFileInfo info(localPath);
            // Drop a folder → expand to its immediate children. This matches
            // what most users expect (dragging a folder in should add the
            // folder's contents, not just one "path/to/folder" string).
            if (info.isDir())
            {
                QDir dir(localPath);
                const QStringList entries = dir.entryList(QDir::Files);
                for (const QString& name : entries)
                {
                    paths << dir.absoluteFilePath(name);
                }
            }
            else if (info.isFile())
            {
                paths << localPath;
            }
        }
    }
    if (paths.isEmpty())
    {
        return;
    }
    event->acceptProposedAction();
    addFilesAndAutoRoute(paths);
    LOG_INFO("MainWindow", QString("Dropped %1 file(s) onto main window").arg(paths.size()));
}
