#!/bin/bash

# Windows Python wheels build script

set -e -x

PYDIR=$(ls -d /c/hostedtoolcache/windows/Python/3.8.*)
export PATH="$PYDIR/x64:$PYDIR/x64/Scripts:$PATH"
echo "PATH=$PATH"

export CMAKE_GENERATOR="Unix Makefiles"
export CMAKE_PREFIX_PATH="C:/smelibs;C:/smelibs/CMake;C:/smelibs/lib/cmake;C:/smelibs/dune"
export SME_EXTRA_EXE_LIBS="-static;-static-libgcc;-static-libstdc++"
export CMAKE_CXX_COMPILER_LAUNCHER=ccache
export CCACHE_NOHASHDIR="true"
export SME_EXTERNAL_SMECORE=on
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
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/c/smelibs -DBUILD_BENCHMARKS=off -DBUILD_CLI=off -DBUILD_TESTING=off -DBUILD_PYTHON_LIBRARY=off -DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH -DSME_EXTRA_EXE_LIBS=$SME_EXTRA_EXE_LIBS -DCMAKE_CXX_COMPILER_LAUNCHER=$CMAKE_CXX_COMPILER_LAUNCHER -DSME_EXTRA_CORE_DEFS=$SME_EXTRA_CORE_DEFS -DSME_EXTERNAL_SMECORE=off
make -j2 core
make install
cd ..

python -m pip install cibuildwheel==$CIBUILDWHEEL_VERSION
python -m cibuildwheel --output-dir wheelhouse

ccache -s
