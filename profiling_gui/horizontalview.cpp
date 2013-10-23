// Copyright (c) 2013 Maciej Gajewski

#include "profiling_gui/horizontalview.hpp"

#include <QMouseEvent>
#include <QDebug>

namespace profiling_gui {

static const int DRAG_THRESHOLD = 10; // in px

HorizontalView::HorizontalView(QWidget *parent) :
    QGraphicsView(parent)
{
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

void HorizontalView::zoomOut()
{
    // TODO
}

void HorizontalView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        _dragging = true;
        _dragStartPx = event->pos().x();
        _dragStart = mapToScene(event->pos()).x();
    }
}

void HorizontalView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && _dragging)
    {
        _dragging = false;
        if (qAbs(event->pos().x() - _dragStartPx) > DRAG_THRESHOLD)
        {
            scene()->invalidate();

            _viewStart = _dragStart;
            _viewEnd = mapToScene(event->pos()).x();
            updateTransformation();
        }
    }
}

void HorizontalView::mouseMoveEvent(QMouseEvent* event)
{
    if (_dragging)
    {
        _dragEnd = mapToScene(event->pos()).x();
        if (qAbs(event->pos().x() - _dragStartPx) > DRAG_THRESHOLD)
        {
            scene()->invalidate();
        }
    }
}

void HorizontalView::drawForeground(QPainter* painter, const QRectF& rect)
{
    if (_dragging)
    {
        // selection frame
        QGraphicsScene* s = scene();
        QRectF sceneRect = s->sceneRect();
        QRectF selectionRect(_dragStart,sceneRect.top(), _dragEnd - _dragStart, sceneRect.height());

        painter->fillRect(selectionRect, QColor(0, 0, 128, 32));
        qDebug() << "drawing selection: " << selectionRect;
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
    if (d > 0)
    {
        // zoom in
        _viewStart = _viewStart + (mousePos-_viewStart)/3.0;
        _viewEnd = _viewEnd - (_viewEnd-mousePos)/3.0;
        updateTransformation();
    }
    else if (d < 0)
    {
        // zoom out
        QRectF sceneRect = scene()->sceneRect();
        _viewStart = qMax(_viewStart- (mousePos-_viewStart)/2.0, sceneRect.left());
        _viewEnd = qMin(_viewEnd + (_viewEnd-mousePos)/3.0, sceneRect.right());
        updateTransformation();
    }
}

void HorizontalView::updateTransformation()
{
    QRectF sceneRect = scene()->sceneRect();
    QRectF visibleRect(_viewStart, sceneRect.top(), _viewEnd - _viewStart, sceneRect.height());
    fitInView(visibleRect);
}

}
