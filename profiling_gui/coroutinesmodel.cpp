// Copyright (c) 2013 Maciej Gajewski

#include "profiling_gui/coroutinesmodel.hpp"

#include <QPixmap>
#include <QPainter>
#include <QDebug>

namespace profiling_gui {

CoroutinesModel::CoroutinesModel(QObject *parent) :
    QAbstractListModel(parent),

    _selectionModel(this)
{
    connect(&_selectionModel, SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), SLOT(onSelectionChanged(QModelIndex)));
}

void CoroutinesModel::Append(const CoroutinesModel::Record& record)
{
    beginInsertRows(QModelIndex(), _records.size(), _records.size());

    _records.append(record);

    endInsertRows();
}

QVariant CoroutinesModel::data(const QModelIndex& index, int role) const
{
    // no nested data
    if (index.parent().isValid())
    {
        return QVariant();
    }

    const Record& record = _records[index.row()];

    if (index.column() == 0)
    {
        if (role == Qt::DisplayRole)
            return record.name;
        else if (role == Qt::DecorationRole)
            return iconFromColor(record.color);
    }
    if (index.column() == 1 && role == Qt::DisplayRole)
    {
        return nanosToString(record.timeExecuted);
    }

    return QVariant();
}

QVariant CoroutinesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch(section)
        {
            case 0:
                return "Name";
            case 1:
                return "Time";
            default:
                ;
        }
    }

    return QVariant();
}

void CoroutinesModel::onCoroutineSelected(quintptr id)
{
    auto it = std::find_if(_records.begin(), _records.end(), [id](const Record& r) { return r.id == id; });
    int row = std::distance(_records.begin(), it);

    blockSignals(true);
    _selectionModel.setCurrentIndex(index(row), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
    blockSignals(false);
}

void CoroutinesModel::onSelectionChanged(QModelIndex index)
{
    emit coroSelected(_records[index.row()].id);
}

QPixmap CoroutinesModel::iconFromColor(QColor color)
{
    static const int ICON_SIZE = 16;
    QPixmap icon(ICON_SIZE, ICON_SIZE);
    icon.fill(Qt::transparent);
    QPainter painter(&icon);

    painter.setPen(QPen(Qt::black, 1));
    painter.setBrush(color);
    painter.drawRect(1, 1, ICON_SIZE-2, ICON_SIZE-2);

    return icon;
}

QString CoroutinesModel::nanosToString(double ns)
{
    static const char* suffixes[] = { "ns", "µs", "ms", "s" };

    double value = ns;
    auto it = std::begin(suffixes);
    for(; it != std::end(suffixes)-1 && value > 1000.0; it++)
        value /= 1000;

    return QString::number(value, 'f', 2) + *it;
}

}
