#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QApplication::setApplicationName("LinksFlow");
    QApplication::setOrganizationName("LinksFlow");

    MainWindow w;
    w.show();
    return QApplication::exec();
}
