// Copyright (c) 2013 Maciej Gajewski

#ifndef PROFILING_GUI_FLOWDIAGRAM_HPP
#define PROFILING_GUI_FLOWDIAGRAM_HPP

#include "profiling_gui/coroutinesmodel.hpp"

#include "profiling_reader/reader.hpp"

#include <QGraphicsScene>
#include <QMap>

#include <limits>

namespace profiling_gui {

// Flow diagram builder
class FlowDiagram : public QObject
{
    Q_OBJECT
public:

    explicit FlowDiagram(QObject *parent = nullptr);
    
    void loadFile(const QString& path, QGraphicsScene* scene, CoroutinesModel& coroutinesModel);

private:

    struct ThreadData
    {
        double y;
        double minTime;
        double maxTime;
        qint64 lastBlock = std::numeric_limits<quint64>::min();
    };

    struct CoroutineData
    {
        QString name;
        QColor color;

        QMap<std::size_t, qint64> enters;
        QPointF lastExit;
        QList<QGraphicsItem*> items;
        quint64 totalTime = 0;
    };

    void onRecord(const profiling_reader::record_type& record);
    double ticksToTime(qint64 ticks) const;

    QGraphicsScene* _scene;
    double _ticksPerNs;
    QMap<std::size_t, ThreadData> _threads;
    QMap<std::uintptr_t, CoroutineData> _coroutines;
};

}

#endif
