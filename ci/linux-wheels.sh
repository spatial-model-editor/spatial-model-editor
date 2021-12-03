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
    -DBUILD_BENCHMARKS=off \
    -DBUILD_GUI=off \
    -DBUILD_CLI=off \
    -DBUILD_TESTING=off \
    -DBUILD_PYTHON_LIBRARY=off \
    -DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH" \
    -DSME_EXTERNAL_SMECORE=off
make -j2 core
make install
cd ..
