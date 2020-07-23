#!/bin/bash
source source.sh

export PATH="/c/Python38:/c/Python38/Scripts:$PATH"
echo "PATH=$PATH"

export CMAKE_GENERATOR="Unix Makefiles"
export CMAKE_PREFIX_PATH="C:/libs;C:/libs/CMake;C:/libs/lib/cmake;C:/libs/dune"
export SME_EXTRA_EXE_LIBS="-static;-static-libgcc;-static-libstdc++"
export SME_EXTRA_CORE_DEFS="_hypot=hypot"
export CMAKE_CXX_COMPILER_LAUNCHER=ccache
export CCACHE_LOGFILE="C:/log.txt"
export CCACHE_NOHASHDIR="true"
export CIBW_BUILD_VERBOSITY=3
export CIBW_BUILD="$CIBW_BUILD"

echo "CIBW_BUILD=$CIBW_BUILD"

pwd
which g++
g++ --version
which cmake
cmake --version
which python
python --version
ccache -s

python -m cibuildwheel --output-dir dist

ccache -s

tail -n 1500 /c/log.txt
