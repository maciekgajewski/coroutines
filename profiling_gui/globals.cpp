// Copyright (c) 2013 Maciej Gajewski

#include "profiling_gui/globals.hpp"

namespace profiling_gui {

QString nanosToString(double ns)
{
    static const char* suffixes[] = { "ns", "µs", "ms", "s" };

    double value = ns;
    auto it = std::begin(suffixes);
    for(; it != std::end(suffixes)-1 && value > 1000.0; it++)
        value /= 1000;

    return QString::number(value, 'f', 2) + *it;
}


} // namespace profiling_gui
