#!/bin/bash

# Ubuntu UBSAN build script

set -e -x

sudo apt-get update -yy
# build dependencies
sudo apt-get install -yy ccache xvfb jwm

# qt dependencies
sudo apt-get install -yy libfontconfig1-dev libfreetype6-dev libx11-dev libx11-xcb-dev libxext-dev libxfixes-dev libxi-dev libxrender-dev libxcb1-dev libxcb-glx0-dev libxcb-keysyms1-dev libxcb-image0-dev libxcb-shm0-dev libxcb-icccm4-dev libxcb-sync-dev libxcb-xfixes0-dev libxcb-shape0-dev libxcb-randr0-dev libxcb-render-util0-dev libxkbcommon-dev libxkbcommon-x11-dev libxcb-xinerama0-dev libkrb5-dev

# use clang 9
sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-9 100
sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-9 100

# check versions
cmake --version
clang++ --version
python --version
ccache --version

ccache --zero-stats

# start a virtual display for the Qt GUI tests
Xvfb -screen 0 1280x800x24 :99 &
export DISPLAY=:99

# start a window manager so the Qt GUI tests can have their focus set
jwm &

# note: currently we just output the UB logs to file, print them and return success:
export UBSAN_OPTIONS="print_stacktrace=1:log_path=$(pwd)/ub"
# todo: once dune has no UB, we can remove the log_path above, and instead add `-fno-sanitize-recover=undefined`
# to the cmake copmile/link flags so that UB results in a failed build

# do build
mkdir build
cd build
CC=clang CXX=clang++ cmake ..  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_PREFIX_PATH="/opt/smelibs;/opt/smelibs/lib/cmake" -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld -fsanitize=undefined -fno-omit-frame-pointer" -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic -Wshadow -Wunused -Wconversion -Wsign-conversion -Wcast-align -fsanitize=undefined -fno-omit-frame-pointer" -DSME_WITH_TBB=ON  -DBoost_NO_BOOST_CMAKE=on -DSTDTHREAD_WORKS=ON
time make tests -j2
ccache --show-stats

# run c++ tests
time ./test/tests -as > tests.txt 2>&1 || (tail -n 1000 tests.txt && exit 1)
tail -n 100 tests.txt

# print UBSAN logfile
cat ../ub* || echo "no UBSAN logfile found"
