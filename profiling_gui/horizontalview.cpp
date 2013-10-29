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
    setCursor(Qt::ArrowCursor);
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
    static int c = 0;
    qDebug() << c++ << "HorizontalView::mousePressEvent, accepted: " << event->isAccepted();
}

void HorizontalView::updateTransformation()
{
    QRectF sceneRect = scene()->sceneRect();
    QRectF visibleRect(_viewStart, sceneRect.top(), _viewEnd - _viewStart, sceneRect.height());
    fitInView(visibleRect);
}

}
