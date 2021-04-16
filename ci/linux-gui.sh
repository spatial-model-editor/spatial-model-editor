#!/bin/bash

# Ubuntu 16.04 GUI/CLI build script

set -e -x

# start a virtual display for the Qt GUI tests
# it can take a while before the display is ready,
# which is why it is the first thing done here
Xvfb -screen 0 1280x800x24 :99 &
export DISPLAY=:99

# static link stdc++ for portable binary
export SME_EXTRA_EXE_LIBS="-static-libgcc;-static-libstdc++"

# do build
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="/opt/smelibs;/opt/smelibs/lib/cmake" -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DSME_EXTRA_EXE_LIBS=$SME_EXTRA_EXE_LIBS -DSME_WITH_TBB=ON
make -j2 VERBOSE=1
ccache --show-stats

# start a window manager so the Qt GUI tests can have their focus set
# note: Xvfb can take a while to start up, which is why jwm is only called now,
# when (hopefully) the Xvfb display will be ready
jwm &

# run cpp tests
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

# display version
./src/spatial-model-editor -v

# move binaries to artefacts/
cd ..
mkdir artefacts
mv build/src/spatial-model-editor artefacts/
mv build/cli/spatial-cli artefacts/
