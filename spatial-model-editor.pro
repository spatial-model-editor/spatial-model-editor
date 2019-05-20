#-------------------------------------------------
#
# Project created by QtCreator 2019-05-06T13:40:04
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = spatial-model-editor
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++11

SOURCES += \
        main.cpp \
        mainwindow.cpp

HEADERS += \
        mainwindow.h

FORMS += \
        mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

LIBS += $$PWD/libsbml/lib/libsbml-static.a $$PWD/libsbml/lib/libexpat.a

INCLUDEPATH += $$PWD/libsbml/include

# to avoid linker errors with libexpat.a (which is not compiled with -fPIC)
# on recent versions of g++ on ubuntu, which default to PIE
#QMAKE_LFLAGS += -no-pie
