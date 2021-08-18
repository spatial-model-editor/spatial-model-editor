#!/bin/bash

# Ubuntu TSAN build script

set -e -x

# start a virtual display for the Qt GUI tests
# it can take a while before the display is ready,
# which is why it is the first thing done here
Xvfb -screen 0 1280x800x24 :99 &
export DISPLAY=:99

# suppress intel TBB false positives
export TSAN_OPTIONS=suppressions=$(pwd)/tsan_suppr.txt
echo "race:^tbb::blocked_range*" > tsan_suppr.txt
echo "race:^tbb::interface9::internal::start_for*" >> tsan_suppr.txt

# do build
mkdir build
cd build
CC=clang CXX=clang++ cmake .. \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_PREFIX_PATH="/opt/smelibs;/opt/smelibs/lib/cmake" \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread -fno-omit-frame-pointer" \
  -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic -Wshadow -Wunused -Wconversion -Wsign-conversion -Wcast-align -fsanitize=thread -fno-omit-frame-pointer" \
  -DSME_WITH_TBB=ON \
  -DBoost_NO_BOOST_CMAKE=on \
  -DSTDTHREAD_WORKS=ON
time make tests -j2
ccache --show-stats

# start a window manager so the Qt GUI tests can have their focus set
# note: Xvfb can take a while to start up, which is why jwm is only called now,
# when (hopefully) the Xvfb display will be ready
jwm &

# run c++ tests
time ./test/tests ~[expensive] -as > tests.txt || echo "ignoring return code for now"
