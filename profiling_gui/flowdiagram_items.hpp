// Copyright (c) 2013 Maciej Gajewski

#ifndef PROFILING_GUI_FLOWDIAGRAM_ITEMS_HPP
#define PROFILING_GUI_FLOWDIAGRAM_ITEMS_HPP

#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QGraphicsObject>

namespace profiling_gui {

// set of custom grpahics items used on flow diagram

// selectable rectagular element
class SelectableRectangle : public QGraphicsRectItem
{
public:

    SelectableRectangle(double l, double t, double w, double h);

    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

protected:

    virtual void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
};

// simple symbol
class SelectableSymbol : public QGraphicsItem
{
public:

    enum SHAPE
    {
        SHAPE_CIRCLE
    };

    SelectableSymbol(const QPointF& pos, SHAPE shape, const QColor color);

    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
    virtual QRectF boundingRect() const override;

private:

    QPointF _pos;
    QColor _color;
    SHAPE _shape;
};


// non-clickable, selctable line
class SelectableLine : public QGraphicsLineItem
{
public:
    SelectableLine(double x1, double y1, double x2, double y2);

    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem * option, QWidget * widget = nullptr) override;
};

// invisible item grouping all items belonging to one coroutine
class CoroutineGroup : public QGraphicsObject
{
    Q_OBJECT
public:

    CoroutineGroup(quintptr id, QGraphicsItem* parent = nullptr);

    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem * option, QWidget * widget) override;
    virtual QRectF boundingRect() const override;

public slots:

    void onCoroutineSelected(quintptr id);

signals:

    void coroSelected(quintptr id);

protected:

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:

    quintptr _id;
};

}

#endif
