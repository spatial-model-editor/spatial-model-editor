#!/bin/bash

# Script to build and install sme::core in the linux docker image before building Python wheels

set -e -x

ls /host

mkdir /project/build
cd /project/build

ccache --show-stats
ccache -p

ccache --max-size 400M
ccache --cleanup
ccache --zero-stats
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/opt/smelibs \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DSME_BUILD_BENCHMARKS=off \
    -DSME_BUILD_GUI=off \
    -DSME_BUILD_CLI=off \
    -DSME_BUILD_TESTS=on \
    -DSME_BUILD_PYTHON_LIBRARY=off \
    -DCMAKE_CXX_FLAGS="-fpic -fvisibility=hidden" \
    -DCMAKE_PREFIX_PATH="/opt/smelibs;/opt/smelibs/lib64/cmake" \
    -DSME_LOG_LEVEL=OFF \
    -DSME_BUILD_CORE=on
make -j4 core tests
ctest -j4 --output-on-failure
make install
ccache --show-stats
cd ..
