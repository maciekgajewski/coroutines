#include <QApplication>

#include <QLabel>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    QLabel* l = new QLabel("hello, world!");
    l->show();

    return app.exec();
}
