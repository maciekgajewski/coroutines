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
    _ui->coroutineView->setModel(&_coroutinesModel);
    _ui->coroutineView->setSelectionModel(_coroutinesModel.selectionModel());

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
    builder.loadFile(path, &_scene, _coroutinesModel);

    _ui->mainView->showAll();
    _ui->coroutineView->resizeColumnToContents(0);
}

}
