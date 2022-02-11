#!/bin/bash

# Windows Python wheels build script

set -e -x

PYDIR=$(ls -d /c/hostedtoolcache/windows/Python/3.9.*)
export PATH="$PYDIR/x64:$PYDIR/x64/Scripts:$PATH"
echo "PATH=$PATH"

export CMAKE_GENERATOR="Unix Makefiles"
export CMAKE_PREFIX_PATH="C:/smelibs;C:/smelibs/CMake;C:/smelibs/lib/cmake"
export SME_EXTRA_EXE_LIBS="-static;-static-libgcc;-static-libstdc++"
# stop Qt from defining UNICODE on windows to avoid dune issues
# used to be opt-in, done by default for Qt >= 6.1.2 see
# https://codereview.qt-project.org/c/qt/qtbase/+/350443
export SME_QT_DISABLE_UNICODE=TRUE
# disable this for now due to "terminate called after throwing an instance of 'core::Error'"
#export CMAKE_CXX_COMPILER_LAUNCHER=ccache
export CCACHE_NOHASHDIR="true"
export SME_BUILD_CORE=off
pwd
which g++
g++ --version
which cmake
cmake --version
which python
python --version

# disable this for now due to "terminate called after throwing an instance of 'core::Error'"
#ccache -s

# Remove CLI11 symlink to itself (causes recursive copying of folders on windows)
rm -rf ext/CLI11/tests/mesonTest/subprojects/*

# build and install sme::core
mkdir build
cd build
cmake .. \
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
    -DSME_QT_DISABLE_UNICODE=$SME_QT_DISABLE_UNICODE \
    -DSME_EXTRA_CORE_DEFS=$SME_EXTRA_CORE_DEFS \
    -DSME_BUILD_CORE=on
make -j2 core tests
ctest -j2
make install
cd ..

python -m pip install cibuildwheel==$CIBUILDWHEEL_VERSION
python -m cibuildwheel --output-dir wheelhouse

# disable this for now due to "terminate called after throwing an instance of 'core::Error'"
#ccache -s
