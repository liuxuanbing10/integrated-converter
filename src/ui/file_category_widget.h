#ifndef FILE_CATEGORY_WIDGET_H
#define FILE_CATEGORY_WIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QList>
#include "format_registry.h"
#include "file_list_widget.h"   // for FileInfo struct

class FileCategoryWidget : public QWidget {
    Q_OBJECT
public:
    explicit FileCategoryWidget(FormatRegistry::Category category, QWidget* parent = nullptr);
    ~FileCategoryWidget() override;

    FormatRegistry::Category category() const { return m_category; }
    QString categoryName() const { return m_categoryName; }

    /// Add files — only those matching this widget's category are accepted.
    /// Returns the count of files actually added.
    int addFiles(const QStringList& filePaths);

    /// Check whether a single file's extension matches this category.
    bool matchesCategory(const QString& filePath) const;

    bool hasFiles() const { return !m_files.isEmpty(); }
    int fileCount() const { return m_files.size(); }
    QList<FileInfo> allFiles() const { return m_files; }
    void clearFiles();

signals:
    void filesChanged();

public slots:
    void onAddFilesClicked();
    void onRemoveSelected();
    void onClearAll();

private:
    void setupUI();
    void setupConnections();
    void updateTable();
    QString formatFileSize(qint64 bytes) const;
    static QString categoryIcon(FormatRegistry::Category cat);

    FormatRegistry::Category m_category;
    QString m_categoryName;
    QList<FileInfo> m_files;

    QTableWidget* m_fileTable;
    QPushButton* m_addBtn;
    QPushButton* m_removeBtn;
    QPushButton* m_clearBtn;
    QLabel* m_infoLabel;
};

#endif // FILE_CATEGORY_WIDGET_H
