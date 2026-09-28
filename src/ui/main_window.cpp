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
#include "settings_page.h"
#include "task_list_widget.h"
#include "task_manager.h"
#include "theme.h"
#include "window_drop.h"

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

    // Page 5: settings (§零.2: extracted to ui/settings_page.{h,cpp})
    {
        SettingsPage* settings = new SettingsPage();
        connect(settings, &SettingsPage::themeModeRequested, this, &MainWindow::setThemeMode);
        m_settingsPage = settings;
    }
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
        const QStringList failed = m_coordinator.failedTaskInputs();
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

    // §零.2: ledger changes (append/prune/clear) mirror into action state.
    connect(&m_coordinator, &ConversionCoordinator::resultsChanged, this, &MainWindow::onResultsChanged);

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

    static constexpr FormatRegistry::Category kAllCats[] = {
        FormatRegistry::Category::Image, FormatRegistry::Category::Document, FormatRegistry::Category::Audio,
        FormatRegistry::Category::Video};

    // Restore previously saved params for each category
    for (auto cat : kAllCats)
    {
        if (m_conversionParams.contains(cat))
        {
            dialog.setParamsForCategory(cat, m_conversionParams[cat]);
        }
    }

    if (dialog.exec() == QDialog::Accepted)
    {
        // Store params from dialog for all categories
        for (auto cat : kAllCats)
        {
            m_conversionParams[cat] = dialog.getParamsForCategory(cat);
        }

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
    m_coordinator.togglePause();
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
        // Jump to the page that received the most files (1..4 == image..video).
        const int counts[] = {imageCount, docCount, audioCount, videoCount};
        int best = 0;
        for (int i = 1; i < 4; ++i)
        {
            if (counts[i] > counts[best])
                best = i;
        }
        onSidebarNav(1 + best);
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
    m_coordinator.clearResults();
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
    m_coordinator.markStarted(taskId);
    m_progressWidget->setCurrentFile(m_coordinator.currentFile());
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
    // §零.2: ledger bookkeeping moved to ConversionCoordinator (resultsChanged
    // signal drives onResultsChanged; onAllTasksCompleted also re-checks).
    m_coordinator.recordCompletedTask(taskId, success);
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
    m_summaryAction->setEnabled(m_coordinator.hasResults());
    m_toolbarStartAction->setEnabled(true);
    m_coordinator.markFinished();
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
    if (m_coordinator.hasResults())
    {
        showConversionSummary();
    }
    LOG_INFO("MainWindow", "所有任务已完成");
}

void MainWindow::onResultsChanged()
{
    m_summaryAction->setEnabled(m_coordinator.hasResults());
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
    // attempt instead of duplicates (§零.2: ledger prune moved to the
    // coordinator; successes stay — they aren't re-run).
    m_coordinator.dropFailedResultsFor(retry);

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
    m_taskStatsLabel->setText(m_coordinator.countersText());
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
    // §零.2: routing/naming/param-merge semantics live in the coordinator +
    // ConversionPlanner; this shell only harvests widget state.
    using Cat = FormatRegistry::Category;

    auto inputFor = [&](FileCategoryWidget* tab, Cat cat) {
        ConversionCoordinator::CategoryInput in;
        in.category = cat;
        in.files = tab->allFiles();
        QVariant saved = m_savedFormats.value(cat);
        in.outputFormat = saved.isValid() ? saved.toString() : m_formatCombo->currentData().toString();
        in.dialogParams = m_conversionParams.value(cat);
        return in;
    };

    const QList<ConversionCoordinator::CategoryInput> inputs = {
        inputFor(m_imageTab, Cat::Image),
        inputFor(m_docTab, Cat::Document),
        inputFor(m_audioTab, Cat::Audio),
        inputFor(m_videoTab, Cat::Video),
    };
    m_coordinator.submit(inputs, m_outputDirEdit->text(), onlyPaths);
}

void MainWindow::showConversionSummary()
{
    BatchConversionSummary summary(this);
    summary.setResults(m_coordinator.results());
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
// Drag-and-drop (payload logic in ui/window_drop.{h,cpp}, §零.2)
// ---------------------------------------------------------------------------
void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (WindowDrop::hasLocalFileUrls(event->mimeData()))
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
    const QStringList paths = WindowDrop::pathsFromDrop(event);
    if (paths.isEmpty())
    {
        return;
    }
    event->acceptProposedAction();
    addFilesAndAutoRoute(paths);
    LOG_INFO("MainWindow", QString("Dropped %1 file(s) onto main window").arg(paths.size()));
}
