#include "mainwindow.h"
#include "core.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setStyle("cleanlooks");
    MainWindow w;
    w.newLogic();
    w.show();


    return a.exec();
}
