# these static libraries are available pre-compiled from
# from https://github.com/lkeegan/libsbml-static
LIBS += \
    $$PWD/libsbml/lib/libsbml-static.a \
    $$PWD/expat/lib/libexpat.a \
    $$PWD/symengine/lib/libsymengine.a \
    $$PWD/gmp/lib/libgmp.a \
    $$PWD/spdlog/lib/spdlog/libspdlog.a

# these static libraries should be compiled first in their directories
LIBS += \
    $$PWD/qcustomplot/libqcustomplot.a \
    $$PWD/triangle/triangle.o \

# include QT and ext headers as system headers to suppress compiler warnings
QMAKE_CXXFLAGS += \
    -isystem "$$PWD/libsbml/include" \
    -isystem "$$PWD/symengine/include" \
    -isystem "$$PWD/gmp/include" \
    -isystem "$$PWD/expat/include" \
    -isystem "$$PWD/qcustomplot" \
    -isystem "$$PWD/triangle" \
    -isystem "$$PWD/spdlog/include" \

