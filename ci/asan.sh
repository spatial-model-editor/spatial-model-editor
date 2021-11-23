#!/bin/bash

# Ubuntu ASAN build script

set -e -x

# start a virtual display for the Qt GUI tests
# it can take a while before the display is ready,
# which is why it is the first thing done here
Xvfb -screen 0 1280x800x24 :99 &
export DISPLAY=:99

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
cmake .. \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_PREFIX_PATH="/opt/smelibs;/opt/smelibs/lib/cmake" \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer" \
    -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic -Wshadow -Wunused -Wconversion -Wsign-conversion -Wcast-align -fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer" \
    -DSME_WITH_TBB=ON \
    -DBoost_NO_BOOST_CMAKE=on \
    -DSTDTHREAD_WORKS=ON \
    -DSME_LOG_LEVEL=TRACE
time make tests -j2
ccache --show-stats

# start a window manager so the Qt GUI tests can have their focus set
# note: Xvfb can take a while to start up, which is why jwm is only called now,
# when (hopefully) the Xvfb display will be ready
jwm &

# run c++ tests
time ./test/tests -as ~[expensive] > tests.txt 2>&1 || (tail -n 1000 tests.txt && exit 1)
tail -n 100 tests.txt

# todo: also run with python lib
# LD_PRELOAD is a hack required to use with python (since python is not itself compiled with ASAN)
# this also requires -shared-asan in link/compile cmake flags
# but also seems to have lots of false positives, so just disabling this for now

# export LD_PRELOAD=$(clang -print-file-name=libclang_rt.asan-x86_64.so)

# run python tests

#cd sme
# python -m pip install -r ../../sme/requirements.txt
#PYTHONMALLOC=malloc python -m unittest discover -s ../../sme/test > sme.txt 2>&1
#tail -n 100 sme.txt
