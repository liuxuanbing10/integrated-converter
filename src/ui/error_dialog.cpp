#include "error_dialog.h"
#include "logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStyle>
#include <QApplication>
#include <QFile>
#include <QDesktopServices>
#include <QUrl>

ErrorDialog::ErrorDialog(QWidget* parent)
    : QDialog(parent)
    , m_selectedAction(Action::Cancel)
    , m_detailsExpanded(false)
{
    setupUi();
}

ErrorDialog::ErrorDialog(const ErrorInfo& error, QWidget* parent)
    : QDialog(parent)
    , m_selectedAction(Action::Cancel)
    , m_detailsExpanded(false)
{
    setupUi();
    setError(error);
}

ErrorDialog::~ErrorDialog() {
}

void ErrorDialog::setupUi() {
    setWindowTitle(tr("错误"));
    setMinimumWidth(450);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    QHBoxLayout* headerLayout = new QHBoxLayout();
    m_iconLabel = new QLabel();
    m_iconLabel->setFixedSize(48, 48);
    m_iconLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    headerLayout->addWidget(m_iconLabel);
    QVBoxLayout* titleMessageLayout = new QVBoxLayout();
    m_titleLabel = new QLabel();
    m_titleLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #d32f2f;");
    m_titleLabel->setWordWrap(true);
    titleMessageLayout->addWidget(m_titleLabel);
    m_messageLabel = new QLabel();
    m_messageLabel->setStyleSheet("font-size: 12px;");
    m_messageLabel->setWordWrap(true);
    titleMessageLayout->addWidget(m_messageLabel);
    headerLayout->addLayout(titleMessageLayout, 1);
    mainLayout->addLayout(headerLayout);
    m_suggestionLabel = new QLabel();
    m_suggestionLabel->setStyleSheet(
        "background-color: #fff3e0; "
        "border: 1px solid #ffb74d; "
        "border-radius: 4px; "
        "padding: 8px; "
        "font-size: 11px;"
    );
    m_suggestionLabel->setWordWrap(true);
    m_suggestionLabel->setVisible(false);
    mainLayout->addWidget(m_suggestionLabel);
    QHBoxLayout* detailsHeaderLayout = new QHBoxLayout();
    m_detailsButton = new QPushButton(tr("显示详细信息"));
    m_detailsButton->setFlat(true);
    m_detailsButton->setStyleSheet("color: #1976d2; text-align: left;");
    connect(m_detailsButton, &QPushButton::clicked, this, &ErrorDialog::onToggleDetails);
    detailsHeaderLayout->addWidget(m_detailsButton);
    detailsHeaderLayout->addStretch();
    mainLayout->addLayout(detailsHeaderLayout);
    m_detailsGroup = new QGroupBox();
    QVBoxLayout* detailsLayout = new QVBoxLayout(m_detailsGroup);
    m_detailsText = new QTextEdit();
    m_detailsText->setReadOnly(true);
    m_detailsText->setMaximumHeight(150);
    m_detailsText->setStyleSheet("font-family: monospace; font-size: 10px;");
    detailsLayout->addWidget(m_detailsText);
    m_detailsGroup->setVisible(false);
    mainLayout->addWidget(m_detailsGroup);
    mainLayout->addStretch();
    m_dontShowAgainCheck = new QCheckBox(tr("不再显示此错误"));
    mainLayout->addWidget(m_dontShowAgainCheck);
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_retryButton = new QPushButton(tr("重试"));
    m_retryButton->setIcon(QIcon(":/icons/refresh.svg"));
    connect(m_retryButton, &QPushButton::clicked, this, &ErrorDialog::onRetryClicked);
    buttonLayout->addWidget(m_retryButton);
    m_ignoreButton = new QPushButton(tr("忽略"));
    connect(m_ignoreButton, &QPushButton::clicked, this, &ErrorDialog::onIgnoreClicked);
    buttonLayout->addWidget(m_ignoreButton);
    m_viewLogButton = new QPushButton(tr("查看日志"));
    m_viewLogButton->setIcon(QIcon(":/icons/detail.svg"));
    connect(m_viewLogButton, &QPushButton::clicked, this, &ErrorDialog::onViewLogClicked);
    buttonLayout->addWidget(m_viewLogButton);
    m_helpButton = new QPushButton(tr("帮助"));
    m_helpButton->setIcon(QIcon(":/icons/help.svg"));
    connect(m_helpButton, &QPushButton::clicked, this, &ErrorDialog::onHelpClicked);
    buttonLayout->addWidget(m_helpButton);
    m_closeButton = new QPushButton(tr("关闭"));
    m_closeButton->setDefault(true);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_closeButton);
    mainLayout->addLayout(buttonLayout);
}

void ErrorDialog::setError(const ErrorInfo& error) {
    m_error = error;
    updateDisplay();
}

void ErrorDialog::updateDisplay() {
    setWindowTitle(formatErrorTitle());
    QStyle::StandardPixmap iconPixmap = QStyle::SP_MessageBoxCritical;
    QString iconPath;
    switch (m_error.code) {
        case ErrorCode::FileNotFound:
        case ErrorCode::UnsupportedFormat:
            iconPixmap = QStyle::SP_MessageBoxWarning;
            iconPath = ":/icons/error.svg";
            break;
        case ErrorCode::TaskCancelled:
            iconPixmap = QStyle::SP_MessageBoxInformation;
            iconPath = ":/icons/help.svg";
            break;
        default:
            iconPixmap = QStyle::SP_MessageBoxCritical;
            iconPath = ":/icons/error.svg";
            break;
    }
    m_iconLabel->setPixmap(QIcon(iconPath).pixmap(48, 48));
    m_titleLabel->setText(formatErrorTitle());
    m_messageLabel->setText(formatErrorMessage());
    QString details = formatErrorDetails();
    if (!details.isEmpty()) {
        m_detailsText->setPlainText(details);
        m_detailsButton->setVisible(true);
    } else {
        m_detailsButton->setVisible(false);
        m_detailsGroup->setVisible(false);
    }
    if (!m_error.suggestion.isEmpty()) {
        m_suggestionLabel->setText(tr("建议: %1").arg(m_error.suggestion));
        m_suggestionLabel->setVisible(true);
    } else {
        m_suggestionLabel->setVisible(false);
    }
    m_retryButton->setVisible(m_error.recoverable);
}

void ErrorDialog::setShowRetryButton(bool show) {
    m_retryButton->setVisible(show && m_error.recoverable);
}

void ErrorDialog::setShowIgnoreButton(bool show) {
    m_ignoreButton->setVisible(show);
}

void ErrorDialog::setShowViewLogButton(bool show) {
    m_viewLogButton->setVisible(show);
}

void ErrorDialog::setShowHelpButton(bool show) {
    m_helpButton->setVisible(show);
}

void ErrorDialog::setShowDontShowAgainOption(bool show) {
    m_dontShowAgainCheck->setVisible(show);
}

bool ErrorDialog::dontShowAgain() const {
    return m_dontShowAgainCheck->isChecked();
}

QString ErrorDialog::formatErrorTitle() const {
    switch (m_error.code) {
        case ErrorCode::FileNotFound: return tr("文件未找到");
        case ErrorCode::PermissionDenied: return tr("权限被拒绝");
        case ErrorCode::DiskSpaceInsufficient: return tr("磁盘空间不足");
        case ErrorCode::ConverterNotFound: return tr("转换器未找到");
        case ErrorCode::ConverterNotAvailable: return tr("转换器不可用");
        case ErrorCode::UnsupportedFormat: return tr("不支持的格式");
        case ErrorCode::ConversionFailed: return tr("转换失败");
        case ErrorCode::TaskCancelled: return tr("任务已取消");
        case ErrorCode::TaskTimeout: return tr("任务超时");
        case ErrorCode::ProcessCrashed: return tr("进程崩溃");
        case ErrorCode::ProcessFailedToStart: return tr("进程启动失败");
        default: return tr("错误");
    }
}

QString ErrorDialog::formatErrorMessage() const {
    return m_error.message;
}

QString ErrorDialog::formatErrorDetails() const {
    QStringList details;
    if (!m_error.details.isEmpty()) {
        details << m_error.details;
    }
    if (!m_error.context.isEmpty()) {
        details << tr("上下文: %1").arg(m_error.context);
    }
    if (!m_error.taskId.isEmpty()) {
        details << tr("任务ID: %1").arg(m_error.taskId);
    }
    if (!m_error.converterName.isEmpty()) {
        details << tr("转换器: %1").arg(m_error.converterName);
    }
    if (!m_error.inputFile.isEmpty()) {
        details << tr("输入文件: %1").arg(m_error.inputFile);
    }
    if (!m_error.outputFile.isEmpty()) {
        details << tr("输出文件: %1").arg(m_error.outputFile);
    }
    if (m_error.retryCount > 0) {
        details << tr("重试次数: %1").arg(m_error.retryCount);
    }
    details << tr("时间: %1").arg(m_error.timestamp.toString("yyyy-MM-dd hh:mm:ss"));
    return details.join("\n");
}

void ErrorDialog::onRetryClicked() {
    m_selectedAction = Action::Retry;
    emit retryRequested();
    accept();
}

void ErrorDialog::onIgnoreClicked() {
    m_selectedAction = Action::Ignore;
    emit ignoreRequested();
    accept();
}

void ErrorDialog::onViewLogClicked() {
    m_selectedAction = Action::ViewLog;
    emit viewLogRequested();
}

void ErrorDialog::onHelpClicked() {
    m_selectedAction = Action::Help;
    emit helpRequested();
    QDesktopServices::openUrl(QUrl("https://github.com/FFmpeg/FFmpeg"));
}

void ErrorDialog::onToggleDetails(bool checked) {
    Q_UNUSED(checked);
    m_detailsExpanded = !m_detailsExpanded;
    m_detailsGroup->setVisible(m_detailsExpanded);
    m_detailsButton->setText(m_detailsExpanded ? tr("隐藏详细信息") : tr("显示详细信息"));
}

ErrorDialog::Action ErrorDialog::showError(QWidget* parent, const ErrorInfo& error,
                                          bool showRetry, bool showIgnore) {
    ErrorDialog dialog(error, parent);
    dialog.setShowRetryButton(showRetry);
    dialog.setShowIgnoreButton(showIgnore);
    dialog.exec();
    return dialog.selectedAction();
}

ErrorDialog::Action ErrorDialog::showErrorWithRetry(QWidget* parent, const ErrorInfo& error) {
    return showError(parent, error, true, true);
}
