#include "file_category_widget.h"
#include "format_registry.h"
#include "logger.h"
#include <QFileInfo>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QDir>

FileCategoryWidget::FileCategoryWidget(FormatRegistry::Category category, QWidget* parent)
    : QWidget(parent)
    , m_category(category)
    , m_fileTable(nullptr)
    , m_addBtn(nullptr)
    , m_removeBtn(nullptr)
    , m_clearBtn(nullptr)
    , m_infoLabel(nullptr)
{
    switch (category) {
        case FormatRegistry::Category::Image:    m_categoryName = tr("图片转换"); break;
        case FormatRegistry::Category::Document:  m_categoryName = tr("文档转换"); break;
        case FormatRegistry::Category::Audio:     m_categoryName = tr("音频转换"); break;
        case FormatRegistry::Category::Video:     m_categoryName = tr("视频转换"); break;
        default:                                  m_categoryName = tr("其他"); break;
    }
    setupUI();
    setupConnections();
}

FileCategoryWidget::~FileCategoryWidget() {
}

void FileCategoryWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(6);

    // ── Category header ──────────────────────────────────────────
    m_infoLabel = new QLabel(categoryIcon(m_category) + tr(" %1 (0 个文件)").arg(m_categoryName));
    m_infoLabel->setStyleSheet(
        "font-size: 15px; font-weight: 600; color: #1664ff; padding: 4px 0;"
    );
    mainLayout->addWidget(m_infoLabel);

    // ── Button row ───────────────────────────────────────────────
    QHBoxLayout* btnRow = new QHBoxLayout();
    m_addBtn = new QPushButton(tr("添加文件"));
    m_addBtn->setStyleSheet(
        "QPushButton { padding: 6px 14px; border: none; "
        "border-radius: 8px; background-color: #1664ff; color: #ffffff; font-size: 13px; font-weight: 600; }"
        "QPushButton:hover { background-color: #0055ff; }"
    );
    m_removeBtn = new QPushButton(tr("移除选中"));
    m_removeBtn->setEnabled(false);
    m_removeBtn->setStyleSheet(
        "QPushButton { padding: 6px 12px; border: 1px solid #dde2e9; border-radius: 8px; "
        "background-color: #ffffff; font-size: 13px; color: #1d2129; }"
        "QPushButton:hover { background-color: #feeced; border-color: #fa9ea3; color: #d7312a; }"
        "QPushButton:disabled { background-color: #f7f9fb; color: #c9cdd4; border-color: #eceded; }"
    );
    m_clearBtn = new QPushButton(tr("清空"));
    m_clearBtn->setEnabled(false);
    m_clearBtn->setStyleSheet(m_removeBtn->styleSheet());
    btnRow->addWidget(m_addBtn);
    btnRow->addWidget(m_removeBtn);
    btnRow->addWidget(m_clearBtn);
    btnRow->addStretch();
    mainLayout->addLayout(btnRow);

    // ── File table ───────────────────────────────────────────────
    m_fileTable = new QTableWidget();
    m_fileTable->setColumnCount(4);
    m_fileTable->setHorizontalHeaderLabels({
        tr("文件名"), tr("大小"), tr("格式"), tr("源路径")
    });
    m_fileTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_fileTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_fileTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_fileTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive);
    m_fileTable->setColumnWidth(1, 80);
    m_fileTable->setColumnWidth(2, 60);
    m_fileTable->setColumnWidth(3, 180);
    m_fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_fileTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_fileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_fileTable->setAlternatingRowColors(true);
    m_fileTable->setStyleSheet(
        "QTableWidget { border: 1px solid #dde2e9; border-radius: 8px; "
        "gridline-color: #eceded; }"
        "QTableWidget::item { padding: 6px; }"
        "QTableWidget::item:selected { background-color: #f3f7ff; color: #1664ff; }"
        "QHeaderView::section { background-color: #f7f9fb; border: none; "
        "border-bottom: 1px solid #dde2e9; padding: 6px; font-weight: 600; color: #4e5969; }"
    );
    mainLayout->addWidget(m_fileTable, 1);
}

void FileCategoryWidget::setupConnections() {
    connect(m_addBtn, &QPushButton::clicked, this, &FileCategoryWidget::onAddFilesClicked);
    connect(m_removeBtn, &QPushButton::clicked, this, &FileCategoryWidget::onRemoveSelected);
    connect(m_clearBtn, &QPushButton::clicked, this, &FileCategoryWidget::onClearAll);
    connect(m_fileTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        int selected = m_fileTable->selectedItems().size() / m_fileTable->columnCount();
        m_removeBtn->setEnabled(selected > 0);
    });
}

bool FileCategoryWidget::matchesCategory(const QString& filePath) const {
    QFileInfo fi(filePath);
    QString ext = fi.suffix().toLower();
    const auto& reg = FormatRegistry::instance();
    switch (m_category) {
        case FormatRegistry::Category::Image:    return reg.isImage(ext);
        case FormatRegistry::Category::Document:  return reg.isDocument(ext);
        case FormatRegistry::Category::Audio:     return reg.isAudio(ext);
        case FormatRegistry::Category::Video:     return reg.isVideo(ext);
        default: return false;
    }
}

int FileCategoryWidget::addFiles(const QStringList& filePaths) {
    int added = 0;
    for (const QString& path : filePaths) {
        if (!matchesCategory(path)) continue;
        bool exists = false;
        for (const FileInfo& fi : m_files) {
            if (fi.filePath == path) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            QFileInfo fi(path);
            FileInfo info;
            info.filePath = path;
            info.fileName = fi.fileName();
            info.fileSize = fi.size();
            info.format = fi.suffix().toLower();
            info.selected = false;
            m_files.append(info);
            added++;
        }
    }
    if (added > 0) {
        updateTable();
        emit filesChanged();
    }
    return added;
}

void FileCategoryWidget::updateTable() {
    m_fileTable->setRowCount(m_files.size());
    for (int i = 0; i < m_files.size(); ++i) {
        const FileInfo& info = m_files[i];
        QTableWidgetItem* nameItem = new QTableWidgetItem(info.fileName);
        nameItem->setData(Qt::UserRole, info.filePath);
        m_fileTable->setItem(i, 0, nameItem);
        m_fileTable->setItem(i, 1, new QTableWidgetItem(formatFileSize(info.fileSize)));
        m_fileTable->setItem(i, 2, new QTableWidgetItem(info.format.toUpper()));
        m_fileTable->setItem(i, 3, new QTableWidgetItem(info.filePath));
    }
    m_infoLabel->setText(categoryIcon(m_category) + tr(" %1 (%2 个文件)").arg(m_categoryName).arg(m_files.size()));
    m_clearBtn->setEnabled(!m_files.isEmpty());
}

void FileCategoryWidget::onAddFilesClicked() {
    const auto& reg = FormatRegistry::instance();
    QString filter;
    switch (m_category) {
        case FormatRegistry::Category::Image:
            filter = reg.fileDialogImageFilter();
            break;
        case FormatRegistry::Category::Document:
            filter = reg.fileDialogDocumentFilter();
            break;
        case FormatRegistry::Category::Audio:
            filter = reg.fileDialogAudioFilter();
            break;
        case FormatRegistry::Category::Video:
            filter = reg.fileDialogVideoFilter();
            break;
        default:
            filter = reg.fileDialogFilter();
            break;
    }
    filter += ";;" + tr("所有文件 (*)");

    QStringList files = QFileDialog::getOpenFileNames(this,
        tr("选择 %1 文件").arg(m_categoryName), QString(), filter);
    if (!files.isEmpty()) {
        addFiles(files);
    }
}

void FileCategoryWidget::onRemoveSelected() {
    QList<int> selectedRows;
    for (QTableWidgetItem* item : m_fileTable->selectedItems()) {
        int row = item->row();
        if (!selectedRows.contains(row)) {
            selectedRows.append(row);
        }
    }
    if (selectedRows.isEmpty()) return;
    std::sort(selectedRows.begin(), selectedRows.end(), std::greater<int>());
    for (int row : selectedRows) {
        m_files.removeAt(row);
    }
    updateTable();
    emit filesChanged();
}

void FileCategoryWidget::onClearAll() {
    if (m_files.isEmpty()) return;
    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("确认"),
        tr("确定要清空 %1 中的所有文件吗？").arg(m_categoryName),
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        m_files.clear();
        updateTable();
        emit filesChanged();
    }
}

void FileCategoryWidget::clearFiles() {
    m_files.clear();
    updateTable();
}

QString FileCategoryWidget::formatFileSize(qint64 bytes) const {
    if (bytes < 1024) {
        return QString("%1 B").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    } else if (bytes < 1024LL * 1024LL * 1024LL) {
        return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    } else {
        return QString("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    }
}

QString FileCategoryWidget::categoryIcon(FormatRegistry::Category cat) {
    switch (cat) {
        case FormatRegistry::Category::Image:    return QString::fromUtf8("\xF0\x9F\x96\xBC");
        case FormatRegistry::Category::Document:  return QString::fromUtf8("\xF0\x9F\x93\x84");
        case FormatRegistry::Category::Audio:     return QString::fromUtf8("\xF0\x9F\x8E\xB5");
        case FormatRegistry::Category::Video:     return QString::fromUtf8("\xF0\x9F\x8E\xAC");
        default: return QString();
    }
}
