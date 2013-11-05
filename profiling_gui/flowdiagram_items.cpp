// Copyright (c) 2013 Maciej Gajewski
#include "profiling_gui/flowdiagram_items.hpp"

#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>

namespace profiling_gui {

SelectableRectangle::SelectableRectangle(double l, double t, double w, double h) : QGraphicsRectItem(l, t, w, h)
{
    setZValue(1.0);
    setFlag(ItemIsSelectable);
}

void SelectableRectangle::paint(QPainter* painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
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


void SelectableRectangle::mousePressEvent(QGraphicsSceneMouseEvent* event)
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

void SelectableRectangle::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    event->ignore();
}


SelectableLine::SelectableLine(double x1, double y1, double x2, double y2) : QGraphicsLineItem(x1, y1, x2, y2)
{
    setZValue(0.0);
    setAcceptedMouseButtons(0);
    setFlag(ItemIsSelectable);
}

void SelectableLine::paint(QPainter* painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
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

CoroutineGroup::CoroutineGroup(quintptr id, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , _id(id)
{
    setFlag(ItemIsSelectable);
}

void CoroutineGroup::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    // invisible
}

QRectF CoroutineGroup::boundingRect() const
{
    return QRectF();
}

void CoroutineGroup::onCoroutineSelected(quintptr id)
{
    if (id == _id)
    {
        scene()->clearSelection();
        blockSignals(true);
        setSelected(true);
        blockSignals(false);
    }
}

QVariant CoroutineGroup::itemChange(GraphicsItemChange change, const QVariant& value)
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

    return QGraphicsObject::itemChange(change, value);
}

SelectableSymbol::SelectableSymbol(const QPointF& pos, SHAPE shape, const QColor color, double size)
    : _pos(pos)
    , _color(color)
    , _shape(shape)
    , _size(size)
{
    setFlag(ItemIsSelectable);
    setFlag(ItemIgnoresTransformations);
    setPos(pos);

    QTransform t;
    t.scale(_size/2, _size/2);
    setTransform(t);
}

void SelectableSymbol::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    QPen p(Qt::black);
    p.setCosmetic(true);
    if (isSelected())
    {
        p.setWidth(2);
    }

    painter->setPen(p);
    painter->setBrush(_color);

    switch (_shape) {
    case SHAPE_CIRCLE:
        painter->drawEllipse(QPointF(0, 0), 1.0, 1.0);
        break;

    case SHAPE_TRIANGLE_LEFT:
        painter->drawPolygon(QPolygonF() << QPointF(0.5, 0.86) << QPointF(-1.0, 0) << QPointF(0.5, -0.86));
        break;

    case SHAPE_TRIANGLE_RIGHT:
        painter->drawPolygon(QPolygonF() << QPointF(-0.5, 0.86) << QPointF(1.0, 0) << QPointF(-0.5, -0.86));
        break;

    default:
        ;
    }
}

QRectF SelectableSymbol::boundingRect() const
{
    return QRectF(-1, -1, 2, 2);
}


}
