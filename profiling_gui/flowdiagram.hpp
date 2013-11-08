// Copyright (c) 2013 Maciej Gajewski

#ifndef PROFILING_GUI_FLOWDIAGRAM_HPP
#define PROFILING_GUI_FLOWDIAGRAM_HPP

#include "profiling_gui/coroutinesmodel.hpp"

#include "profiling_reader/reader.hpp"

#include <QGraphicsScene>
#include <QMap>

#include <random>
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
    };

    struct CoroutineData
    {
        QString name;
        QColor color;

        QMap<std::size_t, std::uint64_t> enters;
        QPointF lastEvent;
        QList<QGraphicsItem*> items;
        double totalTime = 0;
    };

    struct SpinlockData
    {
        QString name;
        QColor color;

        std::uint64_t lastSpinningBeginTime = 0;
        std::size_t lastSpinningBeginThread;
    };

    // first param is time in ns adjusted
    void onRecord(const profiling_reader::record_type& record); // master record dipatcher

    void onCoroutineRecord(const profiling_reader::record_type& record, const ThreadData& thread);
    void onProcessorRecord(const profiling_reader::record_type& record, ThreadData& thread);
    void onSpinlockRecord(const profiling_reader::record_type& record, ThreadData& thread);
    void onMonitorRecord(const profiling_reader::record_type& record, ThreadData& thread);

    QColor randomColor(int baseV = 172);

    QGraphicsScene* _scene;
    QMap<std::size_t, ThreadData> _threads;
    QMap<std::uintptr_t, CoroutineData> _coroutines;
    QMap< std::uintptr_t, SpinlockData> _spinlocks;

    std::minstd_rand0 _random_generator;
};

}

#endif
