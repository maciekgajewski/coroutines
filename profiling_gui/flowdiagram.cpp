// Copyright (c) 2013 Maciej Gajewski

#include "profiling_gui/flowdiagram.hpp"
#include "profiling_gui/flowdiagram_items.hpp"

#include <QDebug>

namespace profiling_gui {

static const double THREAD_Y_SPACING = 100.0;
static const double CORO_H = 5; // half-heights
static const double BLOCK_H = 4;
static const double WAIT_H = 3;




FlowDiagram::FlowDiagram(QObject *parent) :
    QObject(parent)
{
}

void FlowDiagram::loadFile(const QString& path, QGraphicsScene* scene, CoroutinesModel& coroutinesModel)
{
    _random_generator.seed();
    _scene = scene;
    _scene->setSceneRect(QRectF());
    profiling_reader::reader reader(path.toStdString());

    // collect data
    reader.for_each_by_time([this](const profiling_reader::record_type& record)
    {
        this->onRecord(record);
    });

    // build threads
    for(auto it = _threads.begin(); it != _threads.end(); it++)
    {
        ThreadData& thread = it.value();
        auto* item = new QGraphicsLineItem(thread.minTime, thread.y, thread.maxTime, thread.y);
        QPen p(Qt::black);
        p.setCosmetic(true);
        item->setPen(p);
        scene->addItem(item);

        // if there is unfinished block - finish it artificially at the end of thread
        if (thread.lastBlock != 0)
        {
            profiling_reader::record_type fakeRecord;
            fakeRecord.time_ns = thread.maxTime;
            fakeRecord.object_id = it.key();
            fakeRecord.thread_id = it.key();
            fakeRecord.event = "unblock";
            onProcessorRecord(fakeRecord, thread);
        }
    }

    // build coros
    for(auto it = _coroutines.begin(); it != _coroutines.end(); it++)
    {
        const CoroutineData& coro = *it;
        auto* group = new CoroutineGroup(it.key());

        connect(&coroutinesModel, SIGNAL(coroSelected(quintptr)), group, SLOT(onCoroutineSelected(quintptr)));
        connect(group, SIGNAL(coroSelected(quintptr)), &coroutinesModel, SLOT(onCoroutineSelected(quintptr)));

        // if there is open coroutine, finish it at the thread's end
        if (coro.enters.size() == 1)
        {
            auto enterIt = coro.enters.begin();
            const ThreadData& thread = _threads[enterIt.key()];
            // create a fake event
            profiling_reader::record_type fakeRecord;
            fakeRecord.time_ns = thread.maxTime;
            fakeRecord.object_id = it.key();
            fakeRecord.thread_id = enterIt.key();
            fakeRecord.event = "exit";
            onCoroutineRecord(fakeRecord, thread);
        }
        else if (coro.enters.size() > 1)
        {
            qWarning() << "Coroutine withj more than one unfinished enter. id=" << it.key();
        }

        // group all items and add to scene
        for(QGraphicsItem* item : coro.items)
        {
            item->setParentItem(group);
        }
        scene->addItem(group);

        // add to model
        CoroutinesModel::Record r {
            it.key(),
            coro.name,
            coro.color,
            coro.totalTime // time executed, ns
        };
        coroutinesModel.append(r);
    }

    // fix scene rectangle height, s there is half-spacing margin above and below the firsrt and the last thread
    QRectF sceneRect = _scene->sceneRect();
    sceneRect.setTop(-THREAD_Y_SPACING/2);
    sceneRect.setHeight( THREAD_Y_SPACING * _threads.size());
    _scene->setSceneRect(sceneRect);

    // now that we know the scene size, add line for each thread
    for(const ThreadData& thread : _threads)
    {
        auto* item = new QGraphicsLineItem(sceneRect.left(), thread.y, sceneRect.right(), thread.y);
        item->setPen(QPen(Qt::lightGray));
        item->setZValue(-10);
        _scene->addItem(item);
    }
}

QColor FlowDiagram::randomColor(int baseV)
{
    int h = std::uniform_int_distribution<int>(0, 255)(_random_generator);
    int s = 172 + std::uniform_int_distribution<int>(0, 63)(_random_generator);
    int v = baseV + std::uniform_int_distribution<int>(-32, +32)(_random_generator);

    return QColor::fromHsv(h, s, v);
}

void FlowDiagram::onRecord(const profiling_reader::record_type& record)
{
    if (!_threads.contains(record.thread_id))
    {
        ThreadData newThread;
        newThread.minTime = record.time_ns;
        newThread.y = _threads.size() * THREAD_Y_SPACING;

        _threads.insert(record.thread_id, newThread);
    }

    ThreadData& thread = _threads[record.thread_id];
    thread.maxTime = record.time_ns;

    if (record.object_type == "spinlock")
    {
        onSpinlockRecord(record, thread);
    }
    else if (record.object_type == "processor")
    {
        onProcessorRecord(record, thread);
    }
    else if (record.object_type == "coroutine")
    {
        onCoroutineRecord(record, thread);
    }
    else if (record.object_type == "monitor")
    {
        onMonitorRecord(record, thread);
    }
}

void FlowDiagram::onProcessorRecord(const profiling_reader::record_type& record, ThreadData& thread)
{
    if (record.event == "block")
    {
        thread.lastBlock = record.time_ns;
    }

    else if (record.event == "unblock")
    {
        if (thread.lastBlock == 0)
        {
            qWarning() << "Process: unblock withoiut block! id=" << record.object_id << "time=" << record.time_ns;
        }
        else
        {
            double blockX = thread.lastBlock;
            double unblockX = record.time_ns;
            double y = thread.y;

            thread.lastBlock = 0;

            auto* item = new QGraphicsRectItem(blockX, y-BLOCK_H, unblockX-blockX, 2*BLOCK_H);
            QColor c(Qt::lightGray);
            c.setAlpha(128);
            item->setBrush(c);
            item->setToolTip("blocked");
            item->setZValue(2.0);
            _scene->addItem(item);
        }
    }

}

void FlowDiagram::onSpinlockRecord(const profiling_reader::record_type& record, FlowDiagram::ThreadData& thread)
{
    SpinlockData& spinlock = _spinlocks[record.object_id];

    if (!spinlock.color.isValid())
        spinlock.color = randomColor(64);

    if (record.event == "created")
    {
        spinlock.name = QString::fromStdString(record.data);
    }

    else if (record.event == "spinning begin")
    {
        if (spinlock.lastSpinningBeginTime.contains(record.thread_id))
        {
            qWarning() << "Spinlock: 'spinning begin' with one already open! id=" << record.object_id << "time=" << record.time_ns;
        }
        spinlock.lastSpinningBeginTime[record.thread_id] = record.time_ns;
    }

    else if (record.event == "spinning end")
    {
        if (!spinlock.lastSpinningBeginTime.contains(record.thread_id))
        {
            qWarning() << "Spinlock: 'spinning end' without 'spinning begin'! id=" << record.object_id << "time=" << record.time_ns;
        }
        else
        {
            double blockX = spinlock.lastSpinningBeginTime[record.thread_id];
            double unblockX = record.time_ns;
            double y = thread.y;

            spinlock.lastSpinningBeginTime.remove(record.thread_id);

            auto* item = new QGraphicsRectItem(blockX, y-WAIT_H, unblockX-blockX, 2*WAIT_H);
            item->setBrush(spinlock.color);

            QPen p(Qt::red);
            p.setCosmetic(true);
            item->setPen(p);

            if (spinlock.name.isEmpty())
            {
                item->setToolTip(QString("waiting for mutex 0x%1").arg(record.object_id, 0 , 16));
            }
            else
            {
                item->setToolTip(QString("waiting for mutex '%1' (0x%2)").arg(spinlock.name).arg(record.object_id, 0 , 16));
            }
            item->setZValue(2.0);
            _scene->addItem(item);
        }
    }
}

void FlowDiagram::onMonitorRecord(const profiling_reader::record_type& record, FlowDiagram::ThreadData& thread)
{
    QGraphicsPolygonItem* item = nullptr;
    if (record.event == "wait")
    {
        item = new QGraphicsPolygonItem(QPolygonF() << QPointF(0.5, 0.86) << QPointF(-1.0, 0) << QPointF(0.5, -0.86));
        item->setToolTip(QString("wait: %1").arg(QString::fromStdString(record.data)));
    }
    else if (record.event == "wake_all" || record.event == "wake_one")
    {
        item = new QGraphicsPolygonItem(QPolygonF() << QPointF(-0.5, 0.86) << QPointF(1.0, 0) << QPointF(-0.5, -0.86));
        item->setToolTip(QString::fromStdString(record.event));
    }

    if (item)
    {
        item->setPos(record.time_ns, thread.y);
        item->setTransform(QTransform().scale(6, 6));
        item->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        item->setZValue(2.0);
        QPen p(Qt::black);
        p.setCosmetic(true);
        item->setPen(p);
        _scene->addItem(item);
    }
}

void FlowDiagram::onCoroutineRecord(const profiling_reader::record_type& record, const ThreadData& thread)
{
    CoroutineData& coroutine = _coroutines[record.object_id];

    if (!coroutine.color.isValid())
        coroutine.color = randomColor();

    if (record.event == "created")
    {
        coroutine.name = QString::fromStdString(record.data);

        QPointF pos(record.time_ns, thread.y);
        auto* item = new SelectableSymbol(pos, SelectableSymbol::SHAPE_CIRCLE, coroutine.color, 8);
        item->setToolTip(QString("created: " ) + coroutine.name);
        coroutine.items.append(item);
        coroutine.lastEvent = pos;

    }

    else if (record.event == "enter")
    {
        coroutine.enters[record.thread_id] = record.time_ns;
    }

    else if (record.event == "exit")
    {
        if(!coroutine.enters.contains(record.thread_id))
        {
            qWarning() << "Corotuine: exit without enter! id=" << record.object_id << ", time= " << record.time_ns << ",thread=" << record.thread_id;
        }
        else
        {
            double enterX =  coroutine.enters[record.thread_id];
            coroutine.enters.remove(record.thread_id);

            double exitX = record.time_ns;
            double y = thread.y;

            // block
            auto* item = new SelectableRectangle(enterX, y-CORO_H, exitX-enterX, CORO_H*2);
            item->setToolTip(coroutine.name);
            item->setBrush(coroutine.color);

            coroutine.items.append(item);

            // connection with previous one
            if (!coroutine.lastEvent.isNull())
            {
                auto* item = new SelectableLine(coroutine.lastEvent.x(), coroutine.lastEvent.y(), enterX, y);
                QColor c = coroutine.color;
                QPen pen(c);
                item->setPen(pen);
                coroutine.items.append(item);
            }

            coroutine.lastEvent = QPointF(exitX, y);
            coroutine.totalTime += record.time_ns - enterX;
        }
    }
}



}
