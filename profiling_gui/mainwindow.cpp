// Copyright (c) 2013 Maciej Gajewski

#include "profiling_gui/mainwindow.hpp"
#include "profiling_gui/flowdiagram.hpp"

#include "ui_mainwindow.h"

#include <QDebug>

namespace profiling_gui {

MainWindow::MainWindow(QWidget *parent)
:
    QMainWindow(parent),
    _ui(new Ui::MainWindow)
{
    _ui->setupUi(this);
    _ui->mainView->setScene(&_scene);
    connect(_ui->actionExit, SIGNAL(triggered()), QApplication::instance(), SLOT(quit()));
}

MainWindow::~MainWindow()
{
    delete _ui;
}

void MainWindow::loadFile(const QString& path)
{
    FlowDiagram builder;
    _scene.clear();
    builder.loadFile(path, &_scene);

    // scale
    //QRectF boundingRect = _scene.sceneRect();
    //QTransform transformation;
    //transformation.scale(1.0 / (boundingRect.width() / 100E+9), 1.0);
    //transformation.scale(2000.0 / (boundingRect.width()), 1.0);

    //_ui->mainView->setTransform(transformation, false);
    _ui->mainView->showAll();
}

}
