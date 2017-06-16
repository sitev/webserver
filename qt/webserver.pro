QT       -= core gui

TARGET = webserver
TEMPLATE = lib
CONFIG += staticlib

#INCLUDEPATH += ../../core/src
#INCLUDEPATH += ../../network/src


HEADERS += \
    ../src/webserver.h

SOURCES += \
    ../src/webserver.cpp



unix:!macx: LIBS += -L$$PWD/../../core/qt/ -lcore

INCLUDEPATH += $$PWD/../../core/src
DEPENDPATH += $$PWD/../../core/src

unix:!macx: PRE_TARGETDEPS += $$PWD/../../core/qt/libcore.a

unix:!macx: LIBS += -L$$PWD/../../network/qt/ -lnetwork

INCLUDEPATH += $$PWD/../../network/src
DEPENDPATH += $$PWD/../../network/src

unix:!macx: PRE_TARGETDEPS += $$PWD/../../network/qt/libnetwork.a


unix:!macx: LIBS += -L$$PWD/../../mylib -lmylib

INCLUDEPATH += $$PWD/../../mylib
DEPENDPATH += $$PWD/../../mylib

unix:!macx: PRE_TARGETDEPS += $$PWD/../../mylib/libmylib.a
