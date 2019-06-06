QT       += core gui #testlib

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = test
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++11

SOURCES += \
    test/main.cpp \
    test/test_mainwindow.cpp \
    test/test_numerics.cpp \
    test/test_qlabelmousetracker.cpp \
    test/test_sbml.cpp \
    test/test_simulate.cpp \
    src/model.cpp \
    src/numerics.cpp \
    src/sbml.cpp \
    src/simulate.cpp

    # src/mainwindow.cpp \
    # src/qlabelmousetracker.cpp \

HEADERS += \
    inc/model.h \
    inc/numerics.h \
    inc/sbml.h \
    inc/simulate.h \
    test/catch/catch.hpp \
    ext/exprtk/exprtk.hpp \
    test/catch/catch_qt_ostream.h

    # inc/mainwindow.h \
    # inc/qlabelmousetracker.h \

INCLUDEPATH += inc ext/exprtk test/catch

QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS += --coverage
QMAKE_LFLAGS += --coverage

# these static libraries are available from
# from https://github.com/lkeegan/libsbml-static
LIBS += $$PWD/ext/libsbml/lib/libsbml-static.a $$PWD/ext/libsbml/lib/libexpat.a
INCLUDEPATH += ext/libsbml/include

# on windows add flags to support large object files
# https://stackoverflow.com/questions/16596876/object-file-has-too-many-sections
win32: QMAKE_CXXFLAGS += -Wa,-mbig-obj
