#ifndef BATCH_CONVERT_DIALOG_H
#define BATCH_CONVERT_DIALOG_H
#include <QDialog>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
struct FileInfo;
class BatchConvertDialog : public QDialog {
    Q_OBJECT
public:
    explicit BatchConvertDialog(QWidget* parent = nullptr);
    ~BatchConvertDialog() override;
    void setFiles(const QList<FileInfo>& files);
    QString outputDirectory() const;
    QString outputFormat() const;
    bool keepDirectoryStructure() const;
    int namingRule() const;
    bool overwriteExisting() const;
signals:
    void startConversionRequested();
private slots:
    void onBrowseOutputDir();
    void onStart();
    void onCancel();
private:
    void setupUI();
    void setupConnections();
    void updateFilePreview();
    QGroupBox* m_fileListGroup;
    QTableWidget* m_fileTable;
    QLabel* m_fileCountLabel;
    QGroupBox* m_outputGroup;
    QLineEdit* m_outputDirEdit;
    QPushButton* m_browseButton;
    QComboBox* m_outputFormatCombo;
    QCheckBox* m_keepStructureCheck;
    QComboBox* m_namingRuleCombo;
    QCheckBox* m_overwriteCheck;
    QPushButton* m_startButton;
    QPushButton* m_cancelButton;
    QList<FileInfo> m_files;
};
#endif // BATCH_CONVERT_DIALOG_H
