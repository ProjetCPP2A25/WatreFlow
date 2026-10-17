#include "mainwindow.h"
#include <QApplication>
#include <QFont>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("WaterFlow");
    app.setOrganizationName("WaterFlow");

    QFont f;
    f.setFamilies({"Poppins", "Segoe UI", "Noto Sans", "Arial"});
    f.setPointSize(9);
    app.setFont(f);

    MainWindow w;
    w.show();
    return app.exec();
}
