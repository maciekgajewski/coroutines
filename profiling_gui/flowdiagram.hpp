// Copyright (c) 2013 Maciej Gajewski

#ifndef PROFILING_GUI_FLOWDIAGRAM_HPP
#define PROFILING_GUI_FLOWDIAGRAM_HPP

#include <QGraphicsScene>

namespace profiling_gui {

class FlowDiagram : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit FlowDiagram(QObject *parent = nullptr);
    
signals:
    
public slots:
    
};

}

#endif
