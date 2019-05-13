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

win32: LIBS += -L$$PWD/libsbml/win32 -lsbml-static
else:win64: LIBS += -L$$PWD/libsbml/win64 -lsbml-static
else:unix: LIBS += -L$$PWD/libsbml/lib -lsbml-static -lxml2 -lz -lbz2

INCLUDEPATH += $$PWD/libsbml/include
