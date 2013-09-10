TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS += -std=c++11

SOURCES += main.cpp naive_scheduler.cpp
HEADERS += naive_scheduler.hpp \
    naive_channel.hpp
