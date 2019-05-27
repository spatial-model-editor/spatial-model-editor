QT       += core gui

TARGET = test
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++11

SOURCES += \
    test/main.cpp \
    test/test_numerics.cpp \
    test/test_sbml.cpp \
    src/sbml.cpp \
    src/numerics.cpp

HEADERS += \
    inc/sbml.h \
    inc/numerics.h \
    test/catch/catch.hpp \
    ext/exprtk/exprtk.hpp

INCLUDEPATH += inc ext/exprtk test/catch

# these static libraries are available from
# from https://github.com/lkeegan/libsbml-static
LIBS += $$PWD/libsbml/lib/libsbml-static.a $$PWD/libsbml/lib/libexpat.a
INCLUDEPATH += libsbml/include
