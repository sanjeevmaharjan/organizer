#include "ui/MainWindow.h"

#include <QAbstractItemView>
#include <QFileDialog>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <exception>
#include <filesystem>

namespace {

QString displayPath(const std::filesystem::path& path)
{
    return QString::fromStdString(path.string());
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      folderPath_(new QLineEdit(this)),
      previewButton_(new QPushButton(tr("Preview"), this)),
      organizeButton_(new QPushButton(tr("Organize"), this)),
      previewTable_(new QTableWidget(this))
{
    setWindowTitle(tr("Folder Organizer"));
    resize(900, 560);

    auto* centralWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(centralWidget);
    auto* description = new QLabel(
        tr("Sort files in a folder by type. Review the proposed moves before organizing."),
        centralWidget);
    description->setWordWrap(true);
    layout->addWidget(description);

    auto* folderLayout = new QHBoxLayout();
    folderPath_->setPlaceholderText(tr("Choose a folder to organize"));
    auto* browseButton = new QPushButton(tr("Browse…"), centralWidget);
    folderLayout->addWidget(folderPath_, 1);
    folderLayout->addWidget(browseButton);
    layout->addLayout(folderLayout);

    previewTable_->setColumnCount(3);
    previewTable_->setHorizontalHeaderLabels(
        {tr("File"), tr("Type"), tr("Destination")});
    previewTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    previewTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    previewTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    previewTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    previewTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(previewTable_, 1);

    auto* actionLayout = new QHBoxLayout();
    actionLayout->addStretch();
    actionLayout->addWidget(previewButton_);
    actionLayout->addWidget(organizeButton_);
    layout->addLayout(actionLayout);

    setCentralWidget(centralWidget);
    organizeButton_->setEnabled(false);

    connect(browseButton, &QPushButton::clicked, this, &MainWindow::chooseFolder);
    connect(previewButton_, &QPushButton::clicked, this, &MainWindow::preview);
    connect(organizeButton_, &QPushButton::clicked, this, &MainWindow::organize);
    connect(folderPath_, &QLineEdit::textChanged, this, [this] {
        plan_.clear();
        updatePreview();
    });
}

void MainWindow::chooseFolder()
{
    const auto folder = QFileDialog::getExistingDirectory(
        this, tr("Choose folder to organize"), folderPath_->text());
    if (!folder.isEmpty()) {
        folderPath_->setText(folder);
    }
}

void MainWindow::preview()
{
    try {
        plan_ = organizer::Organizer::buildPlan(
            std::filesystem::path(folderPath_->text().toStdString()));
        updatePreview();
        if (plan_.empty()) {
            QMessageBox::information(this, tr("Nothing to organize"),
                                     tr("The selected folder has no files to organize."));
        }
    } catch (const std::exception& exception) {
        plan_.clear();
        updatePreview();
        QMessageBox::critical(this, tr("Unable to preview folder"),
                              QString::fromLocal8Bit(exception.what()));
    }
}

void MainWindow::organize()
{
    if (plan_.empty()) {
        return;
    }

    const auto answer = QMessageBox::question(
        this, tr("Organize files"),
        tr("Move %1 file(s) into type-based subfolders?")
            .arg(static_cast<qulonglong>(plan_.size())),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
    if (answer != QMessageBox::Yes) {
        return;
    }

    const auto result = organizer::Organizer::execute(plan_);
    plan_.clear();
    updatePreview();

    QString summary =
        tr("Moved %1 file(s).").arg(static_cast<qulonglong>(result.completed.size()));
    if (!result.failed.empty()) {
        summary += tr("\n\n%1 file(s) could not be moved:")
                       .arg(static_cast<qulonglong>(result.failed.size()));
        constexpr std::size_t maxShownErrors = 8;
        const auto shown = std::min(result.failed.size(), maxShownErrors);
        for (std::size_t index = 0; index < shown; ++index) {
            summary += QStringLiteral("\n• %1: %2")
                           .arg(displayPath(result.failed[index].source),
                                QString::fromStdString(result.failed[index].reason));
        }
        if (result.failed.size() > shown) {
            summary += tr("\n… and %1 more.")
                           .arg(static_cast<qulonglong>(result.failed.size() - shown));
        }
        QMessageBox::warning(this, tr("Organization completed"), summary);
        return;
    }
    QMessageBox::information(this, tr("Organization completed"), summary);
}

void MainWindow::updatePreview()
{
    previewTable_->setRowCount(static_cast<int>(plan_.size()));
    for (std::size_t index = 0; index < plan_.size(); ++index) {
        const auto& move = plan_[index];
        const auto row = static_cast<int>(index);
        previewTable_->setItem(row, 0,
                               new QTableWidgetItem(displayPath(move.source.filename())));
        previewTable_->setItem(
            row, 1, new QTableWidgetItem(QString::fromStdString(move.category)));
        previewTable_->setItem(row, 2, new QTableWidgetItem(displayPath(move.destination)));
    }
    organizeButton_->setEnabled(!plan_.empty());
}
