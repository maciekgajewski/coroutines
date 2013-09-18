TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS += -std=c++11

SOURCES += main.cpp threaded_scheduler.cpp \
    generator_tests.cpp
HEADERS += threaded_scheduler.hpp \
    threaded_channel.hpp \
    generator.hpp \
    channel.hpp \
    generator_tests.hpp \
    mutex.hpp

INCLUDEPATH += /usr/local/boost_1_54/include

# this crashes
#LIBS += -L/usr/local/boost_1_54/lib -lpthread -lboost_context

# this works
LIBS += -L/usr/local/boost_1_54/lib -lpthread -static -lboost_context
