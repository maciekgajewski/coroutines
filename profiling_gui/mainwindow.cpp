// Copyright (c) 2013 Maciej Gajewski

#include "profiling_gui/mainwindow.hpp"
#include "profiling_gui/flowdiagram.hpp"
#include "profiling_gui/globals.hpp"

#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QSettings>

#include <QDebug>

namespace profiling_gui {

static const int MAX_MRD = 5;

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
//    connect(_ui->actionOpen, SIGNAL(triggered()), SLOT(openFileDialog()));

    connect(_ui->mainView, SIGNAL(rangeHighlighted(uint)), SLOT(timeRangeHighlighted(uint)));

    loadSettings();
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

    // after successful load, add to mrd
    QString absolutePath;
    if (QDir::isAbsolutePath(path))
        absolutePath = path;
    else
        absolutePath = QDir::current().absoluteFilePath(QDir::cleanPath(path));
    _mrd.removeAll(absolutePath);
    _mrd.push_front(absolutePath);
    while (_mrd.size() > MAX_MRD)
        _mrd.pop_back();
    setMrd(_mrd);
}

void MainWindow::on_actionOpen_triggered()
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

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMainWindow::closeEvent(event);
    writeSettings();
}

void MainWindow::loadSettings()
{
    QSettings settings("coroutines", "profiling_gui");

    // mrd list
    _mrd = settings.value("mrd").toStringList();
    setMrd(_mrd);
}

void MainWindow::writeSettings()
{
    QSettings settings("coroutines", "profiling_gui");

    settings.setValue("mrd", _mrd);
}

void MainWindow::setMrd(const QStringList& mrd)
{
    // clear previous actions
    for(QAction* a : _mrdActions)
    {
        delete a;
    }
    _mrdActions.clear();

    if (!mrd.empty())
    {
        _mrdActions.append(_ui->menuFile->insertSeparator(_ui->actionExit));

        for(const QString& file : mrd)
        {
            QAction* action = new QAction(file, _ui->menuFile);
            _ui->menuFile->insertAction(_ui->actionExit, action);

            connect(action, &QAction::triggered, [this, file]() { this->loadFile(file); });
            _mrdActions.append(action);
        }

        _mrdActions.append(_ui->menuFile->insertSeparator(_ui->actionExit));
    }
}

}
