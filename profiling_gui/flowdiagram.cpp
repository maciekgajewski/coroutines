// Copyright (c) 2013 Maciej Gajewski

#include "profiling_gui/flowdiagram.hpp"

#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
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
        qDebug() << "rectange mouse press";
        if (parentItem())
        {
            if (!isSelected())
            {
                qDebug() << "selecting parent";
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

class CoroutineGroup : public QGraphicsItem
{
public:

    CoroutineGroup(QGraphicsItem* parent = nullptr)
    : QGraphicsItem(parent)
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

protected:

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value) override
    {
        if (change == ItemSelectedChange)
        {
            if (value.toBool())
            {
                qDebug() << "parent: selecting al children";
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

};


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
    for(const CoroutineData& coro : _coroutines)
    {
        auto* group = new CoroutineGroup();

        for(QGraphicsItem* item : coro.items)
        {
            item->setParentItem(group);
            //group->addToGroup(item);
        }
        scene->addItem(group);

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
            }
        }
    }
}

}
