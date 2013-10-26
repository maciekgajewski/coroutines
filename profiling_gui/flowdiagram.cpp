// Copyright (c) 2013 Maciej Gajewski

#include "profiling_gui/flowdiagram.hpp"

#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsObject>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>

#include <QDebug>

#include <random>

namespace profiling_gui {

// experimetnal - selectable rectagular element
class SelectableRectangle : public QGraphicsRectItem
{
public:

    SelectableRectangle(double l, double t, double w, double h) : QGraphicsRectItem(l, t, w, h)
    {
        setZValue(1.0);
        setFlag(ItemIsSelectable);
    }

    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0)
    {
        QPen p(Qt::black);
        p.setCosmetic(true);
        if (isSelected())
        {
            p.setWidth(3);
        }

        painter->setPen(p);
        painter->setBrush(brush());

        painter->drawRect(rect());
    }

protected:

    virtual void mousePressEvent(QGraphicsSceneMouseEvent* event)
    {
        if (parentItem())
        {
            if (!isSelected())
            {
                scene()->clearSelection();
                parentItem()->setSelected(true);
            }
        }
        else
        {
            event->ignore();
        }
    }

    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override
    {
        event->ignore();
    }


};

class SelectableLine : public QGraphicsLineItem
{
public:
    SelectableLine(double x1, double y1, double x2, double y2) : QGraphicsLineItem(x1, y1, x2, y2)
    {
        setZValue(0.0);
        setAcceptedMouseButtons(0);
        setFlag(ItemIsSelectable);
    }

    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0)
    {
        QPen p;
        QColor c = pen().color();
        p.setCosmetic(true);
        if (isSelected())
        {
            c.setAlpha(176);
            p.setWidth(2);
        }
        else
        {
            p.setWidth(1);
            c.setAlpha(128);
        }
        p.setColor(c);

        painter->setPen(p);

        painter->drawLine(line());
    }

protected:

};

class CoroutineGroup : public QGraphicsObject
{
    Q_OBJECT
public:

    CoroutineGroup(quintptr id, QGraphicsItem* parent = nullptr)
    : QGraphicsObject(parent)
    , _id(id)
    {
        setFlag(ItemIsSelectable);
    }

    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem * option, QWidget * widget) override
    {
    }

    virtual QRectF boundingRect() const override
    {
        return QRectF();
    }

public slots:

    void onCoroutineSelected(quintptr id)
    {
        if (id == _id)
        {
            scene()->clearSelection();
            blockSignals(true);
            setSelected(true);
            blockSignals(false);
        }
    }

signals:

    void coroSelected(quintptr id);

protected:

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value) override
    {
        if (change == ItemSelectedChange)
        {
            if (value.toBool())
            {
                emit coroSelected(_id);
                for(QGraphicsItem* item : childItems())
                {
                    item->setSelected(true);
                }
            }
            return value;
        }

        return QGraphicsItem::itemChange(change, value);
    }

private:

    quintptr _id;
};

#include "flowdiagram.moc"

FlowDiagram::FlowDiagram(QObject *parent) :
    QObject(parent)
{
}

void FlowDiagram::loadFile(const QString& path, QGraphicsScene* scene, CoroutinesModel& coroutinesModel)
{
    _scene = scene;
    profiling_reader::reader reader(path.toStdString());

    _ticksPerNs = reader.ticks_per_ns();

    // collect data
    reader.for_each_by_time([this](const profiling_reader::record_type& record)
    {
        this->onRecord(record);
    });

    // build threads
    for(const ThreadData& thread : _threads)
    {
        auto* item = new QGraphicsLineItem(thread.minTime, thread.y, thread.maxTime, thread.y);
        QPen p(Qt::black);
        p.setCosmetic(true);
        item->setPen(p);
        scene->addItem(item);
    }

    // build coros
    for(auto it = _coroutines.begin(); it != _coroutines.end(); it++)
    {
        const CoroutineData& coro = *it;
        auto* group = new CoroutineGroup(it.key());

        connect(&coroutinesModel, SIGNAL(coroSelected(quintptr)), group, SLOT(onCoroutineSelected(quintptr)));
        connect(group, SIGNAL(coroSelected(quintptr)), &coroutinesModel, SLOT(onCoroutineSelected(quintptr)));

        for(QGraphicsItem* item : coro.items)
        {
            item->setParentItem(group);
            //group->addToGroup(item);
        }
        scene->addItem(group);

        // add to model
        CoroutinesModel::Record r {
            it.key(),
            coro.name,
            coro.color,
            ticksToTime(coro.totalTime) // time executed, ns
        };
        coroutinesModel.Append(r);
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
            coroutine.enters[record.thread_id] = record.time;
        }

        if (record.event == "exit")
        {
            if(!coroutine.enters.contains(record.thread_id))
            {
                qWarning() << "Corotuine: exit without enter! id=" << record.object_id << ", time= " << record.time << ",thread=" << record.thread_id;
            }
            else
            {
                quint64 enterTicks = coroutine.enters[record.thread_id];
                coroutine.enters.remove(record.thread_id);

                double enterX =  ticksToTime(enterTicks);
                double exitX = ticksToTime(record.time);
                double y = thread.y;

                // block
                auto* item = new SelectableRectangle(enterX, y-CORO_H, exitX-enterX, CORO_H*2);
                item->setToolTip(coroutine.name);
                item->setBrush(coroutine.color);

                coroutine.items.append(item);

                // connection with previous one
                if (!coroutine.lastExit.isNull())
                {
                    auto* item = new SelectableLine(coroutine.lastExit.x(), coroutine.lastExit.y(), enterX, y);
                    QColor c = coroutine.color;
                    QPen pen(c);
                    item->setPen(pen);
                    coroutine.items.append(item);
                }

                coroutine.lastExit = QPointF(exitX, y);
                coroutine.totalTime += record.time - enterTicks;
            }
        }
    }
}

}
