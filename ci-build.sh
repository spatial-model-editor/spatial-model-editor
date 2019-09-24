#!/bin/bash
BUILDTYPE=${1:-"release"}
MAKECMD=${2:-"make"}
MAKEWRAPPER=${3:-""}
NPROCS=${4:-2}

# make sure we get the right mingw64 version of g++ on appveyor
PATH=/mingw64/bin:$PATH
g++ --version
qmake --version
make --version

# on osx, this should make clang compile for target osx>=10.12
export MACOSX_DEPLOYMENT_TARGET=10.12

# compile qcustomplot library
cd ext/qcustomplot
qmake qcustomplot.pro
$MAKECMD -j$NPROCS
ls
cd ../../

# compile triangle library
cd ext/triangle
$MAKECMD -j$NPROCS
cd ../../

# compile unit tests
mkdir build-tests
cd build-tests
qmake ../test.pro CONFIG+=$BUILDTYPE
$MAKECMD -j$NPROCS
cd ../

# compile executable
mkdir build
cd build
qmake ../spatial-model-editor.pro CONFIG+=$BUILDTYPE
$MAKEWRAPPER $MAKECMD -j$NPROCS
cd ../