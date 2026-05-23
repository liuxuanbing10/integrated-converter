#ifndef ERROR_DIALOG_H
#define ERROR_DIALOG_H

#include "error_types.h"
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QCheckBox>
#include <QGroupBox>

class ErrorDialog : public QDialog {
    Q_OBJECT

public:
    enum class Action {
        Retry,
        Ignore,
        ViewLog,
        Help,
        Cancel
    };

    explicit ErrorDialog(QWidget* parent = nullptr);
    ErrorDialog(const ErrorInfo& error, QWidget* parent = nullptr);
    ~ErrorDialog() override;
    void setError(const ErrorInfo& error);
    ErrorInfo error() const { return m_error; }
    void setShowRetryButton(bool show);
    void setShowIgnoreButton(bool show);
    void setShowViewLogButton(bool show);
    void setShowHelpButton(bool show);
    void setShowDontShowAgainOption(bool show);
    bool dontShowAgain() const;
    Action selectedAction() const { return m_selectedAction; }
    static Action showError(QWidget* parent, const ErrorInfo& error,
                           bool showRetry = true, bool showIgnore = true);
    static Action showErrorWithRetry(QWidget* parent, const ErrorInfo& error);

signals:
    void retryRequested();
    void ignoreRequested();
    void viewLogRequested();
    void helpRequested();

private slots:
    void onRetryClicked();
    void onIgnoreClicked();
    void onViewLogClicked();
    void onHelpClicked();
    void onToggleDetails(bool checked);

private:
    void setupUi();
    void updateDisplay();
    QString formatErrorTitle() const;
    QString formatErrorMessage() const;
    QString formatErrorDetails() const;
    ErrorInfo m_error;
    Action m_selectedAction;
    QLabel* m_iconLabel;
    QLabel* m_titleLabel;
    QLabel* m_messageLabel;
    QTextEdit* m_detailsText;
    QLabel* m_suggestionLabel;
    QGroupBox* m_detailsGroup;
    QPushButton* m_detailsButton;
    QPushButton* m_retryButton;
    QPushButton* m_ignoreButton;
    QPushButton* m_viewLogButton;
    QPushButton* m_helpButton;
    QPushButton* m_closeButton;
    QCheckBox* m_dontShowAgainCheck;
    bool m_detailsExpanded;
};

#endif // ERROR_DIALOG_H
