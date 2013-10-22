// Copyright (c) 2013 Maciej Gajewski

#include "profiling_gui/flowdiagram.hpp"

#include <QGraphicsLineItem>

namespace profiling_gui {

FlowDiagram::FlowDiagram(QObject *parent) :
    QObject(parent)
{
}

void FlowDiagram::loadFile(const QString& path, QGraphicsScene* scene)
{
    _scene = scene;
    profiling_reader::reader reader(path.toStdString());

    _ticksPerNs = reader.ticks_per_ns();

    // collect data
    reader.for_each_by_time([this](const profiling_reader::record_type& record)
    {
        this->onRecord(record);
    });

    // build items
    for(const ThreadData& thread : _threads)
    {
        auto* item = new QGraphicsLineItem(thread.minTime, thread.y, thread.maxTime, thread.y);
        scene->addItem(item);
    }
}

double FlowDiagram::ticksToTime(qint64 ticks) const
{
    return ticks / _ticksPerNs;
}

void FlowDiagram::onRecord(const profiling_reader::record_type& record)
{
    static const double THREAD_Y_SPACING = 100.0;

    if (!_threads.contains(record.thread_id))
    {
        ThreadData newThread;
        newThread.minTime = ticksToTime(record.time);
        newThread.y = _threads.size() * THREAD_Y_SPACING;

        _threads.insert(record.thread_id, newThread);
    }

    ThreadData& thread = _threads[record.thread_id];
    thread.maxTime = ticksToTime(record.time);
}

}
