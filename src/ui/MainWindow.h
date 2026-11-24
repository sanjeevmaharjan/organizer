#pragma once

#include "core/Organizer.h"

#include <QMainWindow>

#include <vector>

class QLineEdit;
class QPushButton;
class QTableWidget;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void chooseFolder();
    void preview();
    void organize();
    void updatePreview();

    QLineEdit* folderPath_;
    QPushButton* previewButton_;
    QPushButton* organizeButton_;
    QTableWidget* previewTable_;
    std::vector<organizer::PlannedMove> plan_;
};
