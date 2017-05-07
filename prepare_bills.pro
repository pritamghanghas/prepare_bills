#-------------------------------------------------
#
# Project created by QtCreator 2017-05-05T21:02:23
#
#-------------------------------------------------

QT       += core gui printsupport

QT += webkit #4.8 as pdf printing doesn't work with webengine yet.

#QT += webenginewidgets

#QT += webkitwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = prepare_bills
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui
