#include "batch_conversion_summary.h"
#include <QFileInfo>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QFile>
#include <QDir>
BatchConversionSummary::BatchConversionSummary(QWidget* parent)
    : QDialog(parent)
    , m_statsGroup(nullptr)
    , m_totalLabel(nullptr)
    , m_successLabel(nullptr)
    , m_failedLabel(nullptr)
    , m_timeLabel(nullptr)
    , m_resultTable(nullptr)
    , m_retryButton(nullptr)
    , m_exportCsvButton(nullptr)
    , m_exportTxtButton(nullptr)
    , m_closeButton(nullptr)
    , m_totalCount(0)
    , m_successCount(0)
    , m_failedCount(0)
    , m_totalDuration(0)
{
    setWindowTitle(tr("批量转换结果汇总"));
    resize(800, 600);
    setupUI();
    setupConnections();
}
BatchConversionSummary::~BatchConversionSummary() {
}
void BatchConversionSummary::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    m_statsGroup = new QGroupBox(tr("统计信息"));
    QHBoxLayout* statsLayout = new QHBoxLayout(m_statsGroup);
    m_totalLabel = new QLabel(tr("总计: 0"));
    m_successLabel = new QLabel(tr("成功: 0"));
    m_failedLabel = new QLabel(tr("失败: 0"));
    m_timeLabel = new QLabel(tr("耗时: 0秒"));
    QString labelStyle = "padding: 6px 12px; border-radius: 4px; font-weight: bold;";
    m_totalLabel->setStyleSheet(labelStyle + "background-color: #E3F2FD; color: #1565C0;");
    m_successLabel->setStyleSheet(labelStyle + "background-color: #E8F5E9; color: #2E7D32;");
    m_failedLabel->setStyleSheet(labelStyle + "background-color: #FFEBEE; color: #C62828;");
    m_timeLabel->setStyleSheet(labelStyle + "background-color: #ECEFF1; color: #455A64;");
    statsLayout->addWidget(m_totalLabel);
    statsLayout->addWidget(m_successLabel);
    statsLayout->addWidget(m_failedLabel);
    statsLayout->addStretch();
    statsLayout->addWidget(m_timeLabel);
    mainLayout->addWidget(m_statsGroup);
    QGroupBox* resultGroup = new QGroupBox(tr("转换结果"));
    QVBoxLayout* resultLayout = new QVBoxLayout(resultGroup);
    m_resultTable = new QTableWidget();
    m_resultTable->setColumnCount(5);
    m_resultTable->setHorizontalHeaderLabels({
        tr("状态"), tr("输入文件"), tr("输出文件"), tr("耗时"), tr("错误信息")
    });
    m_resultTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_resultTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
    m_resultTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);
    m_resultTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_resultTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_resultTable->setColumnWidth(0, 60);
    m_resultTable->setColumnWidth(1, 200);
    m_resultTable->setColumnWidth(2, 200);
    m_resultTable->setColumnWidth(3, 80);
    m_resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultTable->setAlternatingRowColors(true);
    m_resultTable->setStyleSheet(
        "QTableWidget {"
        "  border: 1px solid #ccc;"
        "  border-radius: 4px;"
        "  gridline-color: #e0e0e0;"
        "}"
        "QTableWidget::item { padding: 4px; }"
        "QTableWidget::item:selected { background-color: #e3f2fd; color: #000; }"
        "QHeaderView::section {"
        "  background-color: #f5f5f5;"
        "  border: none;"
        "  border-bottom: 1px solid #ccc;"
        "  padding: 4px;"
        "  font-weight: bold;"
        "}"
    );
    resultLayout->addWidget(m_resultTable);
    mainLayout->addWidget(resultGroup, 1);
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_retryButton = new QPushButton(tr("重试失败项"));
    m_retryButton->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    m_retryButton->setEnabled(false);
    m_exportCsvButton = new QPushButton(tr("导出CSV"));
    m_exportCsvButton->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    m_exportTxtButton = new QPushButton(tr("导出文本"));
    m_exportTxtButton->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    m_closeButton = new QPushButton(tr("关闭"));
    m_closeButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    buttonLayout->addWidget(m_retryButton);
    buttonLayout->addWidget(m_exportCsvButton);
    buttonLayout->addWidget(m_exportTxtButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_closeButton);
    mainLayout->addLayout(buttonLayout);
    setStyleSheet(
        "QGroupBox {"
        "  font-weight: bold;"
        "  border: 1px solid #ccc;"
        "  border-radius: 4px;"
        "  margin-top: 8px;"
        "  padding-top: 8px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left: 10px;"
        "  padding: 0 5px;"
        "}"
        "QPushButton {"
        "  padding: 6px 12px;"
        "  border: 1px solid #ccc;"
        "  border-radius: 4px;"
        "  background-color: #fff;"
        "}"
        "QPushButton:hover { background-color: #f0f0f0; }"
        "QPushButton:pressed { background-color: #e0e0e0; }"
        "QPushButton:disabled { background-color: #f5f5f5; color: #999; }"
    );
}
void BatchConversionSummary::setupConnections() {
    connect(m_retryButton, &QPushButton::clicked, this, &BatchConversionSummary::onRetryFailed);
    connect(m_exportCsvButton, &QPushButton::clicked, this, &BatchConversionSummary::onExportCsv);
    connect(m_exportTxtButton, &QPushButton::clicked, this, &BatchConversionSummary::onExportTxt);
    connect(m_closeButton, &QPushButton::clicked, this, &BatchConversionSummary::onClose);
}
void BatchConversionSummary::setResults(const QList<ConversionResult>& results) {
    m_results = results;
    updateDisplay();
}
void BatchConversionSummary::addResult(const ConversionResult& result) {
    m_results.append(result);
    updateDisplay();
}
void BatchConversionSummary::clearResults() {
    m_results.clear();
    m_totalCount = 0;
    m_successCount = 0;
    m_failedCount = 0;
    m_totalDuration = 0;
    updateDisplay();
}
void BatchConversionSummary::updateDisplay() {
    updateStatistics();
    m_resultTable->setRowCount(m_results.size());
    for (int i = 0; i < m_results.size(); ++i) {
        const ConversionResult& result = m_results[i];
        QFileInfo inputInfo(result.inputPath);
        QFileInfo outputInfo(result.outputPath);
        QTableWidgetItem* statusItem = new QTableWidgetItem(
            result.success ? tr("成功") : tr("失败")
        );
        if (result.success) {
            statusItem->setBackground(QColor(232, 245, 233));
            statusItem->setForeground(QColor(46, 125, 50));
        } else {
            statusItem->setBackground(QColor(255, 235, 238));
            statusItem->setForeground(QColor(198, 40, 40));
        }
        m_resultTable->setItem(i, 0, statusItem);
        m_resultTable->setItem(i, 1, new QTableWidgetItem(inputInfo.fileName()));
        m_resultTable->setItem(i, 2, new QTableWidgetItem(outputInfo.fileName()));
        m_resultTable->setItem(i, 3, new QTableWidgetItem(formatDuration(result.durationMs)));
        m_resultTable->setItem(i, 4, new QTableWidgetItem(result.errorMessage));
    }
    m_retryButton->setEnabled(m_failedCount > 0);
}
void BatchConversionSummary::updateStatistics() {
    m_totalCount = m_results.size();
    m_successCount = 0;
    m_failedCount = 0;
    m_totalDuration = 0;
    for (const ConversionResult& result : m_results) {
        if (result.success) {
            ++m_successCount;
        } else {
            ++m_failedCount;
        }
        m_totalDuration += result.durationMs;
    }
    m_totalLabel->setText(tr("总计: %1").arg(m_totalCount));
    m_successLabel->setText(tr("成功: %1").arg(m_successCount));
    m_failedLabel->setText(tr("失败: %1").arg(m_failedCount));
    m_timeLabel->setText(tr("耗时: %1").arg(formatDuration(m_totalDuration)));
}
QString BatchConversionSummary::formatDuration(qint64 ms) const {
    if (ms < 1000) {
        return QString("%1ms").arg(ms);
    } else if (ms < 60000) {
        return QString("%1秒").arg(ms / 1000.0, 0, 'f', 1);
    } else if (ms < 3600000) {
        int min = ms / 60000;
        int sec = (ms % 60000) / 1000;
        return QString("%1分%2秒").arg(min).arg(sec);
    } else {
        int hour = ms / 3600000;
        int min = (ms % 3600000) / 60000;
        return QString("%1时%2分").arg(hour).arg(min);
    }
}
QList<ConversionResult> BatchConversionSummary::failedResults() const {
    QList<ConversionResult> failed;
    for (const ConversionResult& result : m_results) {
        if (!result.success) {
            failed.append(result);
        }
    }
    return failed;
}
void BatchConversionSummary::onRetryFailed() {
    QList<QString> failedPaths;
    for (const ConversionResult& result : m_results) {
        if (!result.success) {
            failedPaths.append(result.inputPath);
        }
    }
    if (!failedPaths.isEmpty()) {
        emit retryRequested(failedPaths);
        accept();
    }
}
void BatchConversionSummary::onExportCsv() {
    QString filePath = QFileDialog::getSaveFileName(this, tr("导出CSV"),
        QDir::homePath() + "/conversion_report.csv",
        tr("CSV文件 (*.csv)")
    );
    if (filePath.isEmpty()) return;
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("错误"), tr("无法创建文件: %1").arg(filePath));
        return;
    }
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << "状态,输入文件,输出文件,耗时(ms),错误信息\n";
    for (const ConversionResult& result : m_results) {
        QString status = result.success ? "成功" : "失败";
        QString input = result.inputPath;
        QString output = result.outputPath;
        QString duration = QString::number(result.durationMs);
        QString error = result.errorMessage;
        input.replace("\"", "\"\"");
        output.replace("\"", "\"\"");
        error.replace("\"", "\"\"");
        out << QString("\"%1\",\"%2\",\"%3\",\"%4\",\"%5\"\n")
                   .arg(status).arg(input).arg(output).arg(duration).arg(error);
    }
    file.close();
    QMessageBox::information(this, tr("成功"), tr("报告已导出到: %1").arg(filePath));
}
void BatchConversionSummary::onExportTxt() {
    QString filePath = QFileDialog::getSaveFileName(this, tr("导出文本"),
        QDir::homePath() + "/conversion_report.txt",
        tr("文本文件 (*.txt)")
    );
    if (filePath.isEmpty()) return;
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("错误"), tr("无法创建文件: %1").arg(filePath));
        return;
    }
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << "========================================\n";
    out << "        批量转换结果汇总报告\n";
    out << "========================================\n\n";
    out << QString("生成时间: %1\n\n").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    out << "统计信息:\n";
    out << QString("  总计: %1\n").arg(m_totalCount);
    out << QString("  成功: %1\n").arg(m_successCount);
    out << QString("  失败: %1\n").arg(m_failedCount);
    out << QString("  总耗时: %1\n\n").arg(formatDuration(m_totalDuration));
    out << "详细结果:\n";
    out << "----------------------------------------\n";
    for (int i = 0; i < m_results.size(); ++i) {
        const ConversionResult& result = m_results[i];
        out << QString("[%1] %2\n").arg(i + 1).arg(result.success ? "成功" : "失败");
        out << QString("  输入: %1\n").arg(result.inputPath);
        out << QString("  输出: %1\n").arg(result.outputPath);
        out << QString("  耗时: %1\n").arg(formatDuration(result.durationMs));
        if (!result.success) {
            out << QString("  错误: %1\n").arg(result.errorMessage);
        }
        out << "\n";
    }
    file.close();
    QMessageBox::information(this, tr("成功"), tr("报告已导出到: %1").arg(filePath));
}
void BatchConversionSummary::onClose() {
    accept();
}
