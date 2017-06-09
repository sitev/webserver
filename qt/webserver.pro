QT       -= core gui

TARGET = webserver
TEMPLATE = lib
CONFIG += staticlib

INCLUDEPATH += ../../core/src
INCLUDEPATH += ../../network/src


HEADERS += \
    ../src/webServer.h

SOURCES += \
    ../src/webServer.cpp
