// Copyright (c) 2013 Maciej Gajewski

#include "profiling_gui/horizontalview.hpp"

#include <QMouseEvent>
#include <QDebug>

namespace profiling_gui {

static const int DRAG_THRESHOLD = 10; // in px

HorizontalView::HorizontalView(QWidget *parent) :
    QGraphicsView(parent)
{
    //setDragMode(ScrollHandDrag);
    setRenderHint(QPainter::Antialiasing);
    setCursor(Qt::CrossCursor);
}

void HorizontalView::showAll()
{
    QGraphicsScene* s = scene();
    if (s)
    {
        QRectF sceneRect = s->sceneRect();
        _viewStart = sceneRect.left();
        _viewEnd = sceneRect.right();
        updateTransformation();
    }
}

void HorizontalView::paintEvent(QPaintEvent* event)
{
    QGraphicsView::paintEvent(event);

    if (_dragging)
    {
        QPainter painter(viewport());

        QRectF r(_dragStart, 0, _dragEnd-_dragStart, height());

        QPen p(Qt::blue, 1);
        p.setCosmetic(true);
        QColor c(Qt::cyan);
        c.setAlpha(32);

        painter.setPen(Qt::NoPen);
        painter.setBrush(c);
        painter.drawRect(r);

        QLine l1(_dragStart, 0, _dragStart, height());
        QLine l2(_dragEnd, 0, _dragEnd, height());

        painter.setPen(Qt::black);
        painter.drawLine(l1);
        painter.drawLine(l2);
    }
}

void HorizontalView::resizeEvent(QResizeEvent* event)
{
    updateTransformation();
    QGraphicsView::resizeEvent(event);
}

void HorizontalView::wheelEvent(QWheelEvent* event)
{
    int d = event->angleDelta().y();
    double mousePos = mapToScene(event->pos()).x();
    double viewStart =  mapToScene(QPoint(0, 0)).x();
    double viewEnd = mapToScene(QPoint(width(), 0)).x();

    if (d > 0)
    {
        // zoom in
        _viewStart = viewStart + (mousePos-viewStart)/3.0;
        _viewEnd = viewEnd - (viewEnd-mousePos)/3.0;
        updateTransformation();
    }
    else if (d < 0)
    {
        // zoom out
        QRectF sceneRect = scene()->sceneRect();
        _viewStart = qMax(viewStart- (mousePos-viewStart)/2.0, sceneRect.left());
        _viewEnd = qMin(viewEnd + (viewEnd-mousePos)/3.0, sceneRect.right());
        updateTransformation();
    }
}

void HorizontalView::mousePressEvent(QMouseEvent* event)
{
    QGraphicsView::mousePressEvent(event);

    if (event->button() == Qt::LeftButton && !event->isAccepted())
    {
        _dragging = true;
        _dragStart = event->pos().x();
        _dragEnd = _dragStart;
    }
}

void HorizontalView::mouseReleaseEvent(QMouseEvent* event)
{
    if (_dragging)
    {
        _dragging = false;
        viewport()->update();
        emit rangeHighlighted(0);
    }
}

void HorizontalView::mouseMoveEvent(QMouseEvent* event)
{
    if (_dragging)
    {
        if (qAbs(_dragEnd - _dragStart) > 5)
        {
            viewport()->update();

            QPointF startPoint = mapToScene(_dragStart, 0);
            QPointF endPoint = mapToScene(_dragEnd, 0);

            unsigned ns = qAbs(endPoint.x() - startPoint.x());
            emit rangeHighlighted(ns);
        }
        _dragEnd = event->pos().x();
    }
}

void HorizontalView::updateTransformation()
{
    QRectF sceneRect = scene()->sceneRect();
    QRectF visibleRect(_viewStart, sceneRect.top(), _viewEnd - _viewStart, sceneRect.height());
    fitInView(visibleRect);
}

}
