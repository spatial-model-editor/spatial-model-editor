#!/bin/bash

# Script to build and install sme::core in the linux docker image before building Python wheels

set -e -x

mkdir /project/build
cd /project/build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/smelibs -DBUILD_BENCHMARKS=off -DBUILD_CLI=off -DBUILD_TESTING=off -DBUILD_PYTHON_LIBRARY=off -DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH -DSME_EXTERNAL_SMECORE=off
make -j2 core
make install
cd ..
