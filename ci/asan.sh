#!/bin/bash

# Ubuntu ASAN build script

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

# various external libs "leak" memory, don't fail the build because of this

# known LSAN "leaks" from external libraries to suppress
export LSAN_OPTIONS=suppressions=$(pwd)/lsan_suppr.txt
echo "leak:libfontconfig.so" > lsan_suppr.txt
echo "leak:libX11.so" >> lsan_suppr.txt
echo "leak:libdbus-1.so" >> lsan_suppr.txt

# hack to prevent external libs from dlclosing libraries,
# which otherwise results in <module not found> LSAN leaks that cannot be suppressed
# https://github.com/google/sanitizers/issues/89#issuecomment-406316683
echo "#include <stdio.h>" > dlclose.c
echo "int dlclose(void *handle) { return 0; }" >> dlclose.c
clang -shared dlclose.c -o libdlclose.so
export LD_PRELOAD=$(pwd)/libdlclose.so

# add some optional ASAN checks
export ASAN_OPTIONS="detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1"

# do build
mkdir build
cd build
CC=clang CXX=clang++ cmake ..  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_PREFIX_PATH="/opt/smelibs;/opt/smelibs/lib/cmake" -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld -fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer" -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic -Wshadow -Wunused -Wconversion -Wsign-conversion -Wcast-align -fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer" -DSME_WITH_TBB=ON  -DBoost_NO_BOOST_CMAKE=on -DSTDTHREAD_WORKS=ON
time make tests -j2
ccache --show-stats

# run c++ tests
time ./test/tests -as > tests.txt 2>&1 || (tail -n 1000 tests.txt && exit 1)
tail -n 100 tests.txt

# todo: also run with python lib
# LD_PRELOAD is a hack required to use with python (since python is not itself compiled with ASAN)
# this also requires -shared-asan in link/compile cmake flags
# but also seems to have lots of false positives, so just disabling this for now

# export LD_PRELOAD=$(clang -print-file-name=libclang_rt.asan-x86_64.so)

# run python tests

#cd sme
#PYTHONMALLOC=malloc python -m unittest discover -s ../../sme/test > sme.txt 2>&1
#tail -n 100 sme.txt
