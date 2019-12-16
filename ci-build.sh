#!/bin/bash
BUILDTYPE=${1:-"release optimize_full"}
MAKECMD=${2:-"make"}
MAKEWRAPPER=${3:-""}
NPROCS=${4:-2}

# make sure we get the right mingw64 version of g++ on appveyor
PATH=/mingw64/bin:$PATH
g++ --version
qmake --version
make --version

# compile qcustomplot library
cd ext/qcustomplot
qmake qcustomplot.pro CONFIG+="$BUILDTYPE"
$MAKECMD -j$NPROCS
ls
cd ../../

# compile triangle library
cd ext/triangle
$MAKECMD -j$NPROCS
cd ../../

# compile spatial model editor & tests
mkdir build
cd build
qmake ../qmake.pro CONFIG+="$BUILDTYPE"
time $MAKEWRAPPER $MAKECMD -j$NPROCS
cd ../
