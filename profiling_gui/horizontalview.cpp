// Copyright (c) 2013 Maciej Gajewski

#include "profiling_gui/horizontalview.hpp"

#include <QMouseEvent>
#include <QDebug>

namespace profiling_gui {

static const int DRAG_THRESHOLD = 10; // in px

HorizontalView::HorizontalView(QWidget *parent) :
    QGraphicsView(parent)
{
    setDragMode(ScrollHandDrag);
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
