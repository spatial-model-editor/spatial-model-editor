#!/bin/bash

# Script to build and install sme::core in the linux docker image before building Python wheels

set -e -x

ls /host
export CCACHE_DIR=/host/home/runner/work/spatial-model-editor/spatial-model-editor/ccache

ccache -s

mkdir /project/build
cd /project/build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/opt/smelibs \
    -DSME_BUILD_BENCHMARKS=off \
    -DSME_BUILD_GUI=off \
    -DSME_BUILD_CLI=off \
    -DSME_BUILD_TESTS=on \
    -DSME_BUILD_PYTHON_LIBRARY=off \
    -DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH" \
    -DSME_BUILD_CORE=on
make -j2 core tests
ctest -j2
make install
cd ..
