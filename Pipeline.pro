#-------------------------------------------------
#
# Project created by QtCreator 2016-12-16T14:01:12
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Pipeline
TEMPLATE = app


SOURCES += main.cpp\
    core.cpp \
    mainwindow.cpp

HEADERS  += mainwindow.h \
    core.h

FORMS    += mainwindow.ui

DISTFILES += \
    background/y86.png

RC_FILE += icon.rc
