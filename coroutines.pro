TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS += -std=c++11

SOURCES += main.cpp naive_scheduler.cpp
HEADERS += naive_scheduler.hpp \
    naive_channel.hpp \
    generator.hpp

INCLUDEPATH += /usr/local/boost_1_54/include

# this crashes
#LIBS += -L/usr/local/boost_1_54/lib -lpthread -lboost_context

# this works
LIBS += -L/usr/local/boost_1_54/lib -lpthread -static -lboost_context
