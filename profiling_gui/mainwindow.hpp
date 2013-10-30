// Copyright (c) 2013 Maciej Gajewski

#ifndef PROFILING_GUI_MAINWINDOW_HPP
#define PROFILING_GUI_MAINWINDOW_HPP

#include "profiling_gui/coroutinesmodel.hpp"

#include <QMainWindow>
#include <QGraphicsScene>

namespace profiling_gui {

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void loadFile(const QString& path);

private slots:

    void timeRangeHighted(unsigned ns);

private:
    Ui::MainWindow *_ui;
    QGraphicsScene _scene;

    CoroutinesModel _coroutinesModel;
};

}

#endif
