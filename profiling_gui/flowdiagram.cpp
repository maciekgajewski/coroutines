// Copyright (c) 2013 Maciej Gajewski

#include "profiling_gui/flowdiagram.hpp"

#include <QGraphicsLineItem>
#include <QGraphicsRectItem>

#include <QDebug>

#include <random>

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
        QPen pen(Qt::black);
        pen.setCosmetic(true);
        item->setPen(pen);
        scene->addItem(item);
    }
}

double FlowDiagram::ticksToTime(qint64 ticks) const
{
    return ticks / _ticksPerNs;
}

static QColor randomColor()
{
    static std::minstd_rand0 generator;

    int h = std::uniform_int_distribution<int>(0, 255)(generator);
    int s = 172;
    int v = 172;

    return QColor::fromHsv(h, s, v);
}

void FlowDiagram::onRecord(const profiling_reader::record_type& record)
{
    static const double THREAD_Y_SPACING = 100.0;
    static const double CORO_H = 5;

    if (!_threads.contains(record.thread_id))
    {
        ThreadData newThread;
        newThread.minTime = ticksToTime(record.time);
        newThread.y = _threads.size() * THREAD_Y_SPACING;

        _threads.insert(record.thread_id, newThread);
    }

    ThreadData& thread = _threads[record.thread_id];
    thread.maxTime = ticksToTime(record.time);

    // coroutines
    if (record.object_type == "coroutine")
    {
        CoroutineData& coroutine = _coroutines[record.object_id];

        if (!coroutine.color.isValid())
            coroutine.color = randomColor();

        if (record.event == "created")
            coroutine.name = QString::fromStdString(record.data);

        if (record.event == "enter")
        {
            //qWarning() << "Corotuine: enter. id=" << record.object_id << ", time= " << record.time << ",thread=" << record.thread_id;
            coroutine.enters[record.thread_id] = ticksToTime(record.time);
        }

        if (record.event == "exit")
        {
            if(!coroutine.enters.contains(record.thread_id))
            {
                qWarning() << "Corotuine: exit without enter! id=" << record.object_id << ", time= " << record.time << ",thread=" << record.thread_id;
            }
            else
            {
                double enterX =  coroutine.enters[record.thread_id];
                coroutine.enters.remove(record.thread_id);
                double exitX = ticksToTime(record.time);
                double y = thread.y;

                // block
                auto* item = new QGraphicsRectItem(enterX, y-CORO_H, exitX-enterX, CORO_H*2);
                item->setToolTip(coroutine.name);
                item->setBrush(coroutine.color);

                QPen pen(Qt::black);
                pen.setCosmetic(true);
                item->setPen(pen);
                _scene->addItem(item);

                // connection with previous one
                if (!coroutine.lastExit.isNull())
                {
                    auto* item = new QGraphicsLineItem(coroutine.lastExit.x(), coroutine.lastExit.y(), enterX, y);
                    QColor c = coroutine.color;
                    c.setAlpha(128);
                    QPen pen(c);
                    pen.setWidth(2);
                    pen.setCosmetic(true);
                    item->setPen(pen);
                    _scene->addItem(item);
                }

                coroutine.lastExit = QPointF(exitX, y);
            }
        }
    }
}

}
