#ifndef FILE_LIST_WIDGET_H
#define FILE_LIST_WIDGET_H
#include <QWidget>
#include <QTableWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDragLeaveEvent>
#include <QMenu>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
struct FileInfo {
    QString filePath;
    QString fileName;
    qint64 fileSize;
    QString format;
    bool selected;
    FileInfo() : fileSize(0), selected(false) {}
    FileInfo(const QString& path, const QString& name, qint64 size, const QString& fmt)
        : filePath(path), fileName(name), fileSize(size), format(fmt), selected(false) {}
};
class FileListWidget : public QWidget {
    Q_OBJECT
public:
    explicit FileListWidget(QWidget* parent = nullptr);
    ~FileListWidget() override;
    QList<FileInfo> selectedFiles() const;
    QList<FileInfo> allFiles() const;
    int fileCount() const { return m_files.size(); }
    bool isEmpty() const { return m_files.isEmpty(); }
    void clear();
    void addFolder(const QString& folderPath, bool recursive = false);
signals:
    void filesAdded(const QList<FileInfo>& files);
    void filesRemoved(const QList<QString>& filePaths);
    void selectionChanged();
    void fileCountChanged(int count);
public slots:
    void addFiles(const QStringList& filePaths);
    void removeSelectedFiles();
    void selectAll();
    void deselectAll();
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
private slots:
    void onRemoveSelected();
    void onClearAll();
    void onOpenFileLocation();
    void onItemSelectionChanged();
    void onCustomContextMenu(const QPoint& pos);
    void onAddButtonClicked();
    void onAddFolderButtonClicked();
private:
    void setupUI();
    void setupConnections();
    void updateTable();
    void addFileToList(const QString& filePath);
    QString formatFileSize(qint64 bytes) const;
    QString getFileFormat(const QString& filePath) const;
    bool isFileSupported(const QString& filePath) const;
    QStringList scanFolderForFiles(const QString& folderPath, bool recursive) const;
    QTableWidget* m_tableWidget;
    QLabel* m_infoLabel;
    QPushButton* m_addButton;
    QPushButton* m_addFolderButton;
    QPushButton* m_removeButton;
    QPushButton* m_clearButton;
    QCheckBox* m_recursiveCheck;
    QList<FileInfo> m_files;
    bool m_dragActive;
};
#endif // FILE_LIST_WIDGET_H
