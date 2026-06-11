#include "file_list_widget.h"
#include "format_registry.h"
#include <QFileInfo>
#include <QHeaderView>
#include <QMimeData>
#include <QUrl>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QApplication>
#include <QDir>
#include <QDirIterator>
FileListWidget::FileListWidget(QWidget* parent)
    : QWidget(parent)
    , m_tableWidget(nullptr)
    , m_infoLabel(nullptr)
    , m_addButton(nullptr)
    , m_addFolderButton(nullptr)
    , m_removeButton(nullptr)
    , m_clearButton(nullptr)
    , m_recursiveCheck(nullptr)
    , m_dragActive(false)
{
    setAcceptDrops(true);
    setupUI();
    setupConnections();
}
FileListWidget::~FileListWidget() {
}
void FileListWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout* headerLayout = new QHBoxLayout();
    m_infoLabel = new QLabel(tr("待转换文件 (0)"), this);
    m_infoLabel->setStyleSheet("font-weight: bold; color: #333;");
    headerLayout->addWidget(m_infoLabel);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(4);
    m_tableWidget->setHorizontalHeaderLabels({
        tr("文件名"), tr("大小"), tr("格式"), tr("路径")
    });
    m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_tableWidget->setColumnWidth(0, 200);
    m_tableWidget->setColumnWidth(1, 80);
    m_tableWidget->setColumnWidth(2, 60);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setAlternatingRowColors(true);
    m_tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tableWidget->setStyleSheet(
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
    mainLayout->addWidget(m_tableWidget);
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_addButton = new QPushButton(tr("添加文件"), this);
    m_addButton->setIcon(QIcon(":/icons/file.svg"));
    m_addFolderButton = new QPushButton(tr("添加文件夹"), this);
    m_addFolderButton->setIcon(QIcon(":/icons/folder.svg"));
    m_recursiveCheck = new QCheckBox(tr("递归子目录"), this);
    m_removeButton = new QPushButton(tr("移除选中"), this);
    m_removeButton->setIcon(QIcon(":/icons/trash.svg"));
    m_removeButton->setEnabled(false);
    m_clearButton = new QPushButton(tr("清空全部"), this);
    m_clearButton->setIcon(QIcon(":/icons/close.svg"));
    m_clearButton->setEnabled(false);
    QPushButton* undoButton = new QPushButton(tr("撤销"), this);
    undoButton->setEnabled(false);
    undoButton->setObjectName("undoButton");
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_addFolderButton);
    buttonLayout->addWidget(m_recursiveCheck);
    buttonLayout->addSpacing(10);
    buttonLayout->addWidget(m_removeButton);
    buttonLayout->addWidget(m_clearButton);
    buttonLayout->addWidget(undoButton);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);
    setStyleSheet(
        "QPushButton {"
        "  padding: 6px 12px;"
        "  border: 1px solid #ccc;"
        "  border-radius: 4px;"
        "  background-color: #fff;"
        "}"
        "QPushButton:hover { background-color: #f0f0f0; }"
        "QPushButton:pressed { background-color: #e0e0e0; }"
        "QPushButton:disabled { background-color: #f5f5f5; color: #999; }"
        "QCheckBox { spacing: 6px; }"
    );
}
void FileListWidget::setupConnections() {
    connect(m_addButton, &QPushButton::clicked, this, &FileListWidget::onAddButtonClicked);
    connect(m_addFolderButton, &QPushButton::clicked, this, &FileListWidget::onAddFolderButtonClicked);
    connect(m_removeButton, &QPushButton::clicked, this, &FileListWidget::onRemoveSelected);
    connect(m_clearButton, &QPushButton::clicked, this, &FileListWidget::onClearAll);
    QPushButton* undoBtn = findChild<QPushButton*>("undoButton");
    if (undoBtn) {
        connect(undoBtn, &QPushButton::clicked, this, &FileListWidget::undoLastAction);
        connect(this, &FileListWidget::fileCountChanged, this, [undoBtn](int count) {
            undoBtn->setEnabled(count > 0);
        });
    }
    connect(m_tableWidget, &QTableWidget::itemSelectionChanged,
            this, &FileListWidget::onItemSelectionChanged);
    connect(m_tableWidget, &QTableWidget::customContextMenuRequested,
            this, &FileListWidget::onCustomContextMenu);
}
void FileListWidget::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        m_dragActive = true;
        m_tableWidget->setStyleSheet(
            m_tableWidget->styleSheet() +
            "QTableWidget { border: 2px dashed #2196F3; background-color: #e3f2fd; }"
        );
    }
}
void FileListWidget::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}
void FileListWidget::dragLeaveEvent(QDragLeaveEvent* event) {
    Q_UNUSED(event);
    m_dragActive = false;
    m_tableWidget->setStyleSheet(
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
}
void FileListWidget::dropEvent(QDropEvent* event) {
    m_dragActive = false;
    m_tableWidget->setStyleSheet(
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
    QList<QUrl> urls = event->mimeData()->urls();
    QStringList filePaths;
    bool recursive = m_recursiveCheck->isChecked();
    for (const QUrl& url : urls) {
        if (url.isLocalFile()) {
            QString path = url.toLocalFile();
            QFileInfo fi(path);
            if (fi.isFile() && isFileSupported(path)) {
                filePaths.append(path);
            } else if (fi.isDir()) {
                filePaths.append(scanFolderForFiles(path, recursive));
            }
        }
    }
    if (!filePaths.isEmpty()) {
        addFiles(filePaths);
    }
    event->acceptProposedAction();
}
QStringList FileListWidget::scanFolderForFiles(const QString& folderPath, bool recursive) const {
    QStringList result;
    QDirIterator::IteratorFlags flags = recursive
        ? QDirIterator::Subdirectories
        : QDirIterator::NoIteratorFlags;
    QDirIterator it(folderPath, QStringList() << "*.*", QDir::Files, flags);
    while (it.hasNext()) {
        QString filePath = it.next();
        if (isFileSupported(filePath)) {
            result.append(filePath);
        }
    }
    return result;
}
void FileListWidget::addFolder(const QString& folderPath, bool recursive) {
    QStringList files = scanFolderForFiles(folderPath, recursive);
    if (files.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("在指定文件夹中未找到支持的文件"));
        return;
    }
    addFiles(files);
}
void FileListWidget::pushUndoState() {
    m_undoStack.push(m_files);
    if (m_undoStack.size() > 20) {
        m_undoStack.removeFirst();
    }
}

void FileListWidget::popUndoState() {
    if (m_undoStack.isEmpty()) return;
    m_files = m_undoStack.pop();
    updateTable();
    emit fileCountChanged(m_files.size());
}

void FileListWidget::undoLastAction() {
    popUndoState();
}

void FileListWidget::addFiles(const QStringList& filePaths) {
    pushUndoState();
    QList<FileInfo> addedFiles;
    for (const QString& path : filePaths) {
        bool exists = false;
        for (const FileInfo& fi : m_files) {
            if (fi.filePath == path) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            addFileToList(path);
            addedFiles.append(m_files.last());
        }
    }
    if (!addedFiles.isEmpty()) {
        updateTable();
        emit filesAdded(addedFiles);
        emit fileCountChanged(m_files.size());
    }
}
void FileListWidget::addFileToList(const QString& filePath) {
    QFileInfo fi(filePath);
    FileInfo info;
    info.filePath = filePath;
    info.fileName = fi.fileName();
    info.fileSize = fi.size();
    info.format = getFileFormat(filePath);
    info.selected = false;
    m_files.append(info);
}
void FileListWidget::updateTable() {
    m_tableWidget->setRowCount(m_files.size());
    for (int i = 0; i < m_files.size(); ++i) {
        const FileInfo& info = m_files[i];
        QTableWidgetItem* nameItem = new QTableWidgetItem(info.fileName);
        nameItem->setData(Qt::UserRole, info.filePath);
        m_tableWidget->setItem(i, 0, nameItem);
        m_tableWidget->setItem(i, 1, new QTableWidgetItem(formatFileSize(info.fileSize)));
        m_tableWidget->setItem(i, 2, new QTableWidgetItem(info.format.toUpper()));
        m_tableWidget->setItem(i, 3, new QTableWidgetItem(info.filePath));
    }
    m_infoLabel->setText(tr("待转换文件 (%1)").arg(m_files.size()));
    m_clearButton->setEnabled(!m_files.isEmpty());
}
QString FileListWidget::formatFileSize(qint64 bytes) const {
    if (bytes < 1024) {
        return QString("%1 B").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    } else if (bytes < 1024 * 1024 * 1024) {
        return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    } else {
        return QString("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    }
}
QString FileListWidget::getFileFormat(const QString& filePath) const {
    QFileInfo fi(filePath);
    return fi.suffix().toLower();
}
bool FileListWidget::isFileSupported(const QString& filePath) const {
    return FormatRegistry::instance().isSupported(getFileFormat(filePath));
}
void FileListWidget::onAddButtonClicked() {
    const auto& reg = FormatRegistry::instance();
    QString filter = reg.fileDialogFilter() + ";;"
                     + reg.fileDialogVideoFilter() + ";;"
                     + reg.fileDialogAudioFilter() + ";;"
                     + reg.fileDialogDocumentFilter() + ";;"
                     + tr("所有文件 (*)");
    QStringList files = QFileDialog::getOpenFileNames(this, tr("选择文件"),
        QString(), filter);
    if (!files.isEmpty()) {
        addFiles(files);
    }
}
void FileListWidget::onAddFolderButtonClicked() {
    QString folder = QFileDialog::getExistingDirectory(this, tr("选择文件夹"));
    if (!folder.isEmpty()) {
        bool recursive = m_recursiveCheck->isChecked();
        addFolder(folder, recursive);
    }
}
void FileListWidget::onRemoveSelected() {
    QList<int> selectedRows;
    for (QTableWidgetItem* item : m_tableWidget->selectedItems()) {
        int row = item->row();
        if (!selectedRows.contains(row)) {
            selectedRows.append(row);
        }
    }
    if (selectedRows.isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先选择要移除的文件"));
        return;
    }
    std::sort(selectedRows.begin(), selectedRows.end(), std::greater<int>());
    QList<QString> removedPaths;
    for (int row : selectedRows) {
        removedPaths.append(m_files[row].filePath);
        m_files.removeAt(row);
    }
    updateTable();
    emit filesRemoved(removedPaths);
    emit fileCountChanged(m_files.size());
}
void FileListWidget::onClearAll() {
    if (m_files.isEmpty()) return;
    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("确认"),
        tr("确定要清空所有文件吗？"), QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        QList<QString> removedPaths;
        for (const FileInfo& fi : m_files) {
            removedPaths.append(fi.filePath);
        }
        m_files.clear();
        updateTable();
        emit filesRemoved(removedPaths);
        emit fileCountChanged(0);
    }
}
void FileListWidget::onOpenFileLocation() {
    int row = m_tableWidget->currentRow();
    if (row >= 0 && row < m_files.size()) {
        QString path = m_files[row].filePath;
        QFileInfo fi(path);
        QDesktopServices::openUrl(QUrl::fromLocalFile(fi.absolutePath()));
    }
}
void FileListWidget::onItemSelectionChanged() {
    int selectedCount = m_tableWidget->selectedItems().size() / m_tableWidget->columnCount();
    m_removeButton->setEnabled(selectedCount > 0);
    emit selectionChanged();
}
void FileListWidget::onCustomContextMenu(const QPoint& pos) {
    QTableWidgetItem* item = m_tableWidget->itemAt(pos);
    if (!item) return;
    QMenu menu(this);
    menu.addAction(tr("移除选中"), this, &FileListWidget::onRemoveSelected);
    menu.addAction(tr("清空全部"), this, &FileListWidget::onClearAll);
    menu.addSeparator();
    menu.addAction(tr("撤销"), this, &FileListWidget::undoLastAction);
    menu.addSeparator();
    menu.addAction(tr("打开文件位置"), this, &FileListWidget::onOpenFileLocation);
    menu.exec(m_tableWidget->mapToGlobal(pos));
}
QList<FileInfo> FileListWidget::selectedFiles() const {
    QList<FileInfo> result;
    for (QTableWidgetItem* item : m_tableWidget->selectedItems()) {
        int row = item->row();
        if (row >= 0 && row < m_files.size()) {
            bool alreadyAdded = false;
            for (const FileInfo& fi : result) {
                if (fi.filePath == m_files[row].filePath) {
                    alreadyAdded = true;
                    break;
                }
            }
            if (!alreadyAdded) {
                result.append(m_files[row]);
            }
        }
    }
    return result;
}
QList<FileInfo> FileListWidget::allFiles() const {
    return m_files;
}
void FileListWidget::clear() {
    m_files.clear();
    updateTable();
    emit fileCountChanged(0);
}
void FileListWidget::removeSelectedFiles() {
    onRemoveSelected();
}
void FileListWidget::selectAll() {
    m_tableWidget->selectAll();
}
void FileListWidget::deselectAll() {
    m_tableWidget->clearSelection();
}
