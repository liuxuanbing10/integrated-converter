#include "settings_page.h"

#include "config_manager.h"
#include "logger.h"
#include "task_manager.h"
#include "theme.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QThread>
#include <QVBoxLayout>

SettingsPage::SettingsPage(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* pageLayout = new QVBoxLayout(this);
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
        connect(b, &QPushButton::clicked, this, [this, mode]() { emit themeModeRequested(mode); });
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
