#!/bin/bash

# Ubuntu UBSAN build script

set -e -x

# start a virtual display for the Qt GUI tests
# it can take a while before the display is ready,
# which is why it is the first thing done here
Xvfb -screen 0 1280x800x24 :99 &
export DISPLAY=:99

# note: currently we just output the UB logs to file, print them and return success:
export UBSAN_OPTIONS="print_stacktrace=1:log_path=$(pwd)/ub"
# todo: once dune has no UB, we can remove the log_path above, and instead add `-fno-sanitize-recover=undefined`
# to the cmake copmile/link flags so that UB results in a failed build

# do build
mkdir build
cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_PREFIX_PATH="/opt/smelibs;/opt/smelibs/lib/cmake" \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=undefined -fno-omit-frame-pointer" \
    -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic -Wshadow -Wunused -Wconversion -Wsign-conversion -Wcast-align -fsanitize=undefined -fno-omit-frame-pointer -D_GLIBCXX_USE_TBB_PAR_BACKEND=0"
time make tests -j2
ccache --show-stats

# start a window manager so the Qt GUI tests can have their focus set
# note: Xvfb can take a while to start up, which is why jwm is only called now,
# when (hopefully) the Xvfb display will be ready
jwm &

# run c++ tests
# also allow tests to fail, since dune-logging segfaults due to UB
time ./test/tests -as ~[expensive] > tests.txt 2>&1 || (tail -n 1000 tests.txt)
tail -n 100 tests.txt

# print UBSAN logfile
cat ../ub* || echo "no UBSAN logfile found"
