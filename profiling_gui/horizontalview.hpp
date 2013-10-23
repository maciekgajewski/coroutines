// Copyright (c) 2013 Maciej Gajewski

#ifndef PROFILING_GUI_HORIZONTALVIEW_HPP
#define PROFILING_GUI_HORIZONTALVIEW_HPP

#include <QGraphicsView>

namespace profiling_gui {

class HorizontalView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit HorizontalView(QWidget *parent = 0);
    
public slots:

    void showAll();
    void zoomOut();

protected:

    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void drawForeground(QPainter* painter, const QRectF& rect) override;
    virtual void resizeEvent(QResizeEvent* event) override;
    virtual void wheelEvent(QWheelEvent *event) override;

private:

    void updateTransformation();

    double _viewStart;
    double _viewEnd;

    double _dragStart;
    int _dragStartPx;
    double _dragEnd;
    bool _dragging = false;
};

} // namespace profiling_gui

#endif // PROFILING_GUI_HORIZONTALVIEW_HPP
