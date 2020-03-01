TEMPLATE = app
include(../src/core/core.pri)
include(../common.pri)
LIBS += -L../src/core -lcore
include(../ext/ext.pri)

CONFIG += bench
CONFIG += console

bench {
    TARGET = benchmark
    SOURCES += benchmark.cpp
}

pix {
    TARGET = pixel
    SOURCES += pixel.cpp
}
