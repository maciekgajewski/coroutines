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
        std::uint64_t minTime;
        std::uint64_t maxTime;
        std::uint64_t lastBlock = 0;
        std::uint64_t lastLocking = 0;
    };

    struct CoroutineData
    {
        QString name;
        QColor color;

        QMap<std::size_t, double> enters;
        QPointF lastEvent;
        QList<QGraphicsItem*> items;
        double totalTime = 0;
    };

    // first param is time in ns adjusted
    void onRecord(const profiling_reader::record_type& record); // master recvord dipatcher
    void onCoroutineRecord(const profiling_reader::record_type& record, const ThreadData& thread);
    void onProcessorRecord(const profiling_reader::record_type& record, ThreadData& thread);

    QGraphicsScene* _scene;
    QMap<std::size_t, ThreadData> _threads;
    QMap<std::uintptr_t, CoroutineData> _coroutines;
};

}

#endif
