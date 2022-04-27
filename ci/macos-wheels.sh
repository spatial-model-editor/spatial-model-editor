#!/bin/bash

# MacOS 11 Python wheels build script

set -e -x

brew install ccache

export MACOSX_DEPLOYMENT_TARGET=10.14
export CCACHE_BASEDIR=/private
export CMAKE_CXX_COMPILER_LAUNCHER="ccache"
export CMAKE_PREFIX_PATH="/opt/smelibs;/opt/smelibs/lib/cmake"
export SME_BUILD_CORE=off

# check versions
cmake --version
g++ --version
python --version
ccache --zero-stats

# build and install sme::core
mkdir build
cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/opt/smelibs \
    -DBUILD_TESTING=on \
    -DSME_BUILD_BENCHMARKS=off \
    -DSME_BUILD_CLI=off \
    -DSME_BUILD_GUI=off \
    -DSME_BUILD_PYTHON_LIBRARY=off \
    -DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH \
    -DSME_EXTRA_EXE_LIBS=$SME_EXTRA_EXE_LIBS \
    -DCMAKE_CXX_COMPILER_LAUNCHER=$CMAKE_CXX_COMPILER_LAUNCHER \
    -DSME_EXTRA_CORE_DEFS=$SME_EXTRA_CORE_DEFS \
    -DSME_BUILD_CORE=on
make -j3 core tests
ctest -j3
sudo make install
cd ..

python -m pip install cibuildwheel==$CIBUILDWHEEL_VERSION
python -m cibuildwheel --output-dir wheelhouse

ccache -s
