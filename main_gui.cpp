#include <QApplication>

#include "ui/mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("enestkt"));
    QCoreApplication::setApplicationName(QStringLiteral("LogAnalyzerGui"));

    MainWindow window;
    window.show();

    return app.exec();
}