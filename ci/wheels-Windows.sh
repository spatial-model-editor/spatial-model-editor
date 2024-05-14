#!/bin/bash

# Script to build and install sme::core on Windows and use it to build Python wheels

set -e -x

# Use native Windows python
PYDIR=$(ls -d /c/hostedtoolcache/windows/Python/3.12.*)
export PATH="$PYDIR/x64:$PYDIR/x64/Scripts:$PATH"
echo "PATH=$PATH"

export CMAKE_CXX_COMPILER_LAUNCHER=ccache
export CMAKE_GENERATOR="Ninja"
export CMAKE_PREFIX_PATH="C:/smelibs;C:/smelibs/CMake;C:/smelibs/lib/cmake"
export SME_EXTRA_EXE_LIBS="-static;-static-libgcc;-static-libstdc++"
export SME_EXTRA_CORE_DEFS="_hypot=hypot;MS_WIN64"
export CMAKE_ARGS="-DSME_LOG_LEVEL=OFF -DCMAKE_CXX_FLAGS=-fvisibility=hidden -DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH -DSME_BUILD_CORE=off -DSME_EXTRA_EXE_LIBS=$SME_EXTRA_EXE_LIBS -DSME_EXTRA_CORE_DEFS=$SME_EXTRA_CORE_DEFS -DCMAKE_CXX_COMPILER_LAUNCHER=$CMAKE_CXX_COMPILER_LAUNCHER"
pwd
which g++
g++ --version
which cmake
cmake --version
which python
python --version

ccache -s

# Remove CLI11 symlink to itself (causes recursive copying of folders on windows)
rm -rf ext/CLI11/tests/mesonTest/subprojects/*

# build and install sme::core
mkdir build
cd build
cmake .. \
    -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/c/smelibs \
    -DBUILD_TESTING=on \
    -DSME_BUILD_BENCHMARKS=off \
    -DSME_BUILD_CLI=off \
    -DSME_BUILD_GUI=off \
    -DSME_BUILD_PYTHON_LIBRARY=off \
    -DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH \
    -DSME_EXTRA_EXE_LIBS=$SME_EXTRA_EXE_LIBS \
    -DCMAKE_CXX_COMPILER_LAUNCHER=$CMAKE_CXX_COMPILER_LAUNCHER \
    -DCMAKE_CXX_FLAGS="-fpic -fvisibility=hidden" \
    -DSME_EXTRA_CORE_DEFS=$SME_EXTRA_CORE_DEFS \
    -DSME_LOG_LEVEL=OFF \
    -DSME_BUILD_CORE=on
ninja core tests
ctest -j4 --output-on-failure
ninja install
cd ..

ccache -s

python -m pip install cibuildwheel=="$CIBUILDWHEEL_VERSION"
python -m cibuildwheel --output-dir ./artifacts/dist
