#include <QApplication>

#include "profiling_gui/mainwindow.hpp"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    auto window = new profiling_gui::MainWindow();
    window->show();

    if (argc > 1)
    {
        QString path = argv[1];
        window->loadFile(path);
    }

    return app.exec();
}
