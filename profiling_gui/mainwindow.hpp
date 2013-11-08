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

public slots:

    void loadFile(const QString& path);

private slots:

    void on_actionOpen_triggered();
    void timeRangeHighlighted(unsigned ns);

protected:

    virtual void closeEvent(QCloseEvent* event) override;

private:

    void loadSettings();
    void writeSettings();

    // adds most-recent-document list to menu
    void setMrd(const QStringList& mrd);

    Ui::MainWindow *_ui;
    QGraphicsScene _scene;

    CoroutinesModel _coroutinesModel;

    QStringList _mrd;
    QList<QAction*> _mrdActions;
};

}

#endif
