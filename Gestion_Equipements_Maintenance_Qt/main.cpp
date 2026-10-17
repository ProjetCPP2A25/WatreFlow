#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("WaterFlow");
    app.setStyle("Fusion");
    MainWindow window;
    window.show();
    return app.exec();
}
