#!/bin/bash

# Ubuntu 16.04 GUI/CLI build script

set -e -x

# static link stdc++ for portable binary
export SME_EXTRA_EXE_LIBS="-static-libgcc;-static-libstdc++"

sudo apt-get update -yy

# build dependencies
sudo apt-get install -yy ccache xvfb jwm

# qt dependencies
sudo apt-get install -yy libfontconfig1-dev libfreetype6-dev libx11-dev libx11-xcb-dev libxext-dev libxfixes-dev libxi-dev libxrender-dev libxcb1-dev libxcb-glx0-dev libxcb-keysyms1-dev libxcb-image0-dev libxcb-shm0-dev libxcb-icccm4-dev libxcb-sync-dev libxcb-xfixes0-dev libxcb-shape0-dev libxcb-randr0-dev libxcb-render-util0-dev libxkbcommon-dev libxkbcommon-x11-dev libxcb-xinerama0-dev libkrb5-dev

# use gcc 9
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 100
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 100

# check versions
cmake --version
g++ --version
python --version
ccache --zero-stats

# start a virtual display for the Qt GUI tests
Xvfb -screen 0 1280x800x24 :99 &
export DISPLAY=:99

# start a window manager so the Qt GUI tests can have their focus set
jwm &

# do build
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="/opt/smelibs;/opt/smelibs/lib/cmake;/opt/smelibs/dune" -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DSME_EXTRA_EXE_LIBS=$SME_EXTRA_EXE_LIBS -DSME_WITH_TBB=ON
make -j2 VERBOSE=1
ccache --show-stats

# run cpp tests
# skip gui tests as segfaults on github actions
./test/tests -as > tests.txt 2>&1 || (tail -n 1000 tests.txt && exit 1)
tail -n 100 tests.txt

# run python tests
cd sme
python -m unittest discover -s ../../sme/test -v
PYTHONPATH=`pwd` python ../../sme/test/sme_doctest.py -v
cd ..

# run benchmarks (~1 sec per benchmark, ~20secs total)
time ./benchmark/benchmark 1

# check dependencies of binaries
ldd src/spatial-model-editor
ldd cli/spatial-cli

# move binaries to artefacts/
cd ..
mkdir artefacts
mv build/src/spatial-model-editor artefacts/
mv build/cli/spatial-cli artefacts/
