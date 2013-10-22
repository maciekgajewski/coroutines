// Copyright (c) 2013 Maciej Gajewski

#ifndef PROFILING_GUI_FLOWDIAGRAM_HPP
#define PROFILING_GUI_FLOWDIAGRAM_HPP

#include "profiling_reader/reader.hpp"

#include <QGraphicsScene>
#include <QMap>

namespace profiling_gui {

// Flow diagram builder
class FlowDiagram : public QObject
{
    Q_OBJECT
public:

    explicit FlowDiagram(QObject *parent = nullptr);
    
    void loadFile(const QString& path, QGraphicsScene* scene);

private:

    struct ThreadData
    {
        double y;
        double minTime;
        double maxTime;
    };

    void onRecord(const profiling_reader::record_type& record);
    double ticksToTime(qint64 ticks) const;

    QGraphicsScene* _scene;
    double _ticksPerNs;
    QMap<std::size_t, ThreadData> _threads;
};

}

#endif
