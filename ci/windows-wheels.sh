#!/bin/bash

set -e -x

export PATH="/c/hostedtoolcache/windows/Python/3.8.7/x64:/c/hostedtoolcache/windows/Python/3.8.7/x64/Scripts::$PATH"
echo "PATH=$PATH"

export CMAKE_GENERATOR="Unix Makefiles"
export CMAKE_PREFIX_PATH="C:/smelibs;C:/smelibs/CMake;C:/smelibs/lib/cmake;C:/smelibs/dune"
export SME_EXTRA_EXE_LIBS="-static;-static-libgcc;-static-libstdc++"
export CMAKE_CXX_COMPILER_LAUNCHER=ccache
export CCACHE_NOHASHDIR="true"
export CIBW_BUILD_VERBOSITY=3

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

python -m pip install cibuildwheel==$CIBUILDWHEEL_VERSION
python -m cibuildwheel --output-dir wheelhouse

ccache -s
