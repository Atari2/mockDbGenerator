#include "mainwindow.h"

#include <QApplication>
#include <QProcess>
#include <QMessageBox>

static bool check_python_version() {
    QProcess proc{};
    proc.start("py", QStringList{} << "--version");
    if (!proc.waitForFinished()) {
        qDebug() << "Process failed";
        QMessageBox::critical(nullptr, "Failed to check python version", proc.readAllStandardOutput());
        return false;
    }
    auto output{QString{proc.readAllStandardOutput()}.trimmed().toStdString()};
    int major{0}, minor{0}, bugfix{0};
    int read = sscanf_s(output.c_str(), "Python %d.%d.%d", &major, &minor, &bugfix);
    if (read != 3) {
        QMessageBox::critical(nullptr, "Failed to parse python version", QString::fromStdString(output) + " is not a valid python version");
        return false;
    }
    if (!(major >= 3 && minor >= 10 && bugfix >= 8)) {
        QMessageBox::critical(nullptr, "Python version check failed", QString::fromStdString(output) + " is too low, minimum required is 3.10.8");
        return false;
    }
    return true;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    if (!check_python_version()) {
        return EXIT_FAILURE;
    }
    QIcon icon{":/resources/mockDbGeneratorUI.ico"};
    w.setWindowIcon(icon);
    w.show();
    return a.exec();
}
