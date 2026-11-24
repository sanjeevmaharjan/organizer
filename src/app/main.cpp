#include "ui/MainWindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    QApplication::setApplicationName("Folder Organizer");
    QApplication::setOrganizationName("FolderOrganizer");

    MainWindow window;
    window.show();
    return application.exec();
}
