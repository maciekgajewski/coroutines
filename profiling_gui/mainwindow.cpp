// Copyright (c) 2013 Maciej Gajewski

#include "profiling_gui/mainwindow.hpp"
#include "profiling_gui/flowdiagram.hpp"
#include "profiling_gui/globals.hpp"

#include "ui_mainwindow.h"

#include <QFileDialog>

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
    connect(_ui->actionOpen, SIGNAL(triggered()), SLOT(openFileDialog()));

    connect(_ui->mainView, SIGNAL(rangeHighlighted(uint)), SLOT(timeRangeHighlighted(uint)));
}

MainWindow::~MainWindow()
{
    delete _ui;
}

void MainWindow::loadFile(const QString& path)
{
    FlowDiagram builder;
    _scene.clear();
    _coroutinesModel.clear();
    builder.loadFile(path, &_scene, _coroutinesModel);

    _ui->mainView->showAll();
    _ui->coroutineView->resizeColumnToContents(0);
}

void MainWindow::openFileDialog()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open profiling dump file");
    if (!fileName.isNull())
    {
        loadFile(fileName);
    }
}

void MainWindow::timeRangeHighlighted(unsigned ns)
{
    if (ns > 0)
        statusBar()->showMessage(QString("range: %1").arg(nanosToString(ns)));
    else
        statusBar()->clearMessage();
}

}
