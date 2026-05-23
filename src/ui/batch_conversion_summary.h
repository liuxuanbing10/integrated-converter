#ifndef BATCH_CONVERSION_SUMMARY_H
#define BATCH_CONVERSION_SUMMARY_H
#include <QWidget>
#include <QDialog>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDateTime>
struct ConversionResult {
    QString inputPath;
    QString outputPath;
    bool success;
    QString errorMessage;
    qint64 durationMs;
    ConversionResult()
        : success(false)
        , durationMs(0)
    {}
    ConversionResult(const QString& in, const QString& out, bool ok, const QString& err, qint64 dur)
        : inputPath(in)
        , outputPath(out)
        , success(ok)
        , errorMessage(err)
        , durationMs(dur)
    {}
};
class BatchConversionSummary : public QDialog {
    Q_OBJECT
public:
    explicit BatchConversionSummary(QWidget* parent = nullptr);
    ~BatchConversionSummary() override;
    void setResults(const QList<ConversionResult>& results);
    void addResult(const ConversionResult& result);
    void clearResults();
    int totalCount() const { return m_totalCount; }
    int successCount() const { return m_successCount; }
    int failedCount() const { return m_failedCount; }
    QList<ConversionResult> failedResults() const;
signals:
    void retryRequested(const QList<QString>& inputPaths);
    void exportRequested(const QString& format);
private slots:
    void onRetryFailed();
    void onExportCsv();
    void onExportTxt();
    void onClose();
private:
    void setupUI();
    void setupConnections();
    void updateDisplay();
    void updateStatistics();
    QString formatDuration(qint64 ms) const;
    QGroupBox* m_statsGroup;
    QLabel* m_totalLabel;
    QLabel* m_successLabel;
    QLabel* m_failedLabel;
    QLabel* m_timeLabel;
    QTableWidget* m_resultTable;
    QPushButton* m_retryButton;
    QPushButton* m_exportCsvButton;
    QPushButton* m_exportTxtButton;
    QPushButton* m_closeButton;
    QList<ConversionResult> m_results;
    int m_totalCount;
    int m_successCount;
    int m_failedCount;
    qint64 m_totalDuration;
};
#endif // BATCH_CONVERSION_SUMMARY_H
