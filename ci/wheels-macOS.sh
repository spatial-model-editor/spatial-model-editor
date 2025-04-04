#!/bin/bash

# Script to build and install sme::core on macOS, then use it to build Python wheels

set -e -x

# temporary workaround for cmake 4.0 complaining about symengine min cmake version being too low:
export CMAKE_POLICY_VERSION_MINIMUM=3.5

export CCACHE_BASEDIR=/private
export CMAKE_GENERATOR="Ninja"
export CMAKE_CXX_COMPILER_LAUNCHER="ccache"
export CMAKE_PREFIX_PATH="/opt/smelibs;/opt/smelibs/lib/cmake"
export CMAKE_ARGS="-DSME_LOG_LEVEL=OFF -DCMAKE_CXX_FLAGS=-fvisibility=hidden -DCMAKE_OSX_DEPLOYMENT_TARGET=$MACOSX_DEPLOYMENT_TARGET -DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH -DSME_BUILD_CORE=off -DCMAKE_CXX_COMPILER_LAUNCHER=$CMAKE_CXX_COMPILER_LAUNCHER"

# workaround for assumption made by some older pip versions that macOS version is always 10.x
export SYSTEM_VERSION_COMPAT=0

# check versions
cmake --version
g++ --version
python --version

# build and install sme::core
mkdir build
cd build
cmake .. \
    -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/opt/smelibs \
    -DBUILD_TESTING=on \
    -DSME_BUILD_BENCHMARKS=off \
    -DSME_BUILD_CLI=off \
    -DSME_BUILD_GUI=off \
    -DSME_BUILD_PYTHON_LIBRARY=off \
    -DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH \
    -DSME_EXTRA_EXE_LIBS="$SME_EXTRA_EXE_LIBS" \
    -DCMAKE_CXX_COMPILER_LAUNCHER=$CMAKE_CXX_COMPILER_LAUNCHER \
    -DCMAKE_CXX_FLAGS="-fvisibility=hidden" \
    -DSME_EXTRA_CORE_DEFS="$SME_EXTRA_CORE_DEFS" \
    -DSME_LOG_LEVEL=OFF \
    -DSME_BUILD_CORE=on
ninja core tests
ctest -j4 --output-on-failure
sudo ninja install
cd ..

ccache -s

python -m pip install cibuildwheel=="$CIBUILDWHEEL_VERSION"
python -m cibuildwheel --output-dir ./artifacts/dist
