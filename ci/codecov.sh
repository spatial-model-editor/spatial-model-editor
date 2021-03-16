#!/bin/bash

# Ubuntu 16.04 Tests Coverage build script

set -e -x

sudo apt-get update -yy

# build dependencies
sudo apt-get install -yy ccache xvfb jwm lcov

# qt dependencies
sudo apt-get install -yy libfontconfig1-dev libfreetype6-dev libx11-dev libx11-xcb-dev libxext-dev libxfixes-dev libxi-dev libxrender-dev libxcb1-dev libxcb-glx0-dev libxcb-keysyms1-dev libxcb-image0-dev libxcb-shm0-dev libxcb-icccm4-dev libxcb-sync-dev libxcb-xfixes0-dev libxcb-shape0-dev libxcb-randr0-dev libxcb-render-util0-dev libxkbcommon-dev libxkbcommon-x11-dev libxcb-xinerama0-dev libkrb5-dev

# use clang 9
sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-9 100
sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-9 100
sudo update-alternatives --install /usr/bin/llvm-cov llvm-cov /usr/bin/llvm-cov-9 100

# check versions
cmake --version
clang++ --version
llvm-cov --version
python --version
ccache --version

ccache --zero-stats

# start a virtual display for the Qt GUI tests
Xvfb -screen 0 1280x800x24 :99 &
export DISPLAY=:99

# start a window manager so the Qt GUI tests can have their focus set
jwm &

# do build
mkdir build
cd build
CC=clang CXX=clang++ cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="/opt/smelibs;/opt/smelibs/lib/cmake" -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld --coverage" -DCMAKE_CXX_FLAGS="--coverage" -DSME_WITH_TBB=ON -DBoost_NO_BOOST_CMAKE=on
make -j2 VERBOSE=1
ccache --show-stats

# run tests and collect coverage data
mkdir gcov

# core unit core_tests
lcov -q -z -d .
./test/tests -as "[core]" > core.txt 2>&1 || (tail -n 1000 core.txt && exit 1)
tail -n 100 core.txt
llvm-cov gcov -p src/core/CMakeFiles/core.dir/*/src/*.gcno > /dev/null
llvm-cov gcov -p test/CMakeFiles/tests.dir/__/src/core/*/src/*.gcno > /dev/null
mv *#src#core*.gcov gcov/
rm -f *.gcov
ls gcov
# upload coverage report to codecov.io
bash <(curl --connect-timeout 10 --retry 5 -s https://codecov.io/bash) -X gcov -F core

# gui unit tests
rm -f gcov/*
lcov -q -z -d .
./test/tests -as "~[mainwindow][gui]" > gui.txt 2>&1 || (tail -n 1000 gui.txt && exit 1)
tail -n 100 gui.txt
llvm-cov gcov -p src/core/CMakeFiles/core.dir/*/src/*.gcno > /dev/null
llvm-cov gcov -p test/CMakeFiles/tests.dir/__/src/core/*/src/*.gcno > /dev/null
llvm-cov gcov -p src/gui/CMakeFiles/gui.dir/*.gcno > /dev/null
llvm-cov gcov -p src/gui/CMakeFiles/gui.dir/*/*.gcno > /dev/null
mv *#src#core*.gcov *#src#gui*.gcov gcov/
rm -f *.gcov
# upload coverage report to codecov.io
bash <(curl --connect-timeout 10 --retry 5 -s https://codecov.io/bash) -X gcov -F gui

# mainwindow unit tests
rm -f gcov/*
lcov -q -z -d .
./test/tests -as "[mainwindow][gui]" > gui-mainwindow.txt 2>&1 || (tail -n 1000 gui-mainwindow.txt && exit 1)
tail -n 100 gui-mainwindow.txt
llvm-cov gcov -p src/core/CMakeFiles/core.dir/*/src/*.gcno > /dev/null
llvm-cov gcov -p test/CMakeFiles/tests.dir/__/src/core/*/src/*.gcno > /dev/null
llvm-cov gcov -p src/gui/CMakeFiles/gui.dir/*.gcno > /dev/null
llvm-cov gcov -p src/gui/CMakeFiles/gui.dir/*/*.gcno > /dev/null
mv *#src#core*.gcov *#src#gui*.gcov gcov/
rm -f *.gcov
# upload coverage report to codecov.io
bash <(curl --connect-timeout 10 --retry 5 -s https://codecov.io/bash) -X gcov -F mainwindow

# cli unit tests
rm -f gcov/*
lcov -q -z -d .
./test/tests -as "[cli]" > cli.txt 2>&1 || (tail -n 1000 cli.txt && exit 1)
tail -n 100 cli.txt
llvm-cov gcov -p src/core/CMakeFiles/core.dir/*/src/*.gcno > /dev/null
llvm-cov gcov -p cli/CMakeFiles/cli.dir/src/*.gcno > /dev/null
llvm-cov gcov -p test/CMakeFiles/tests.dir/__/src/core/*/src/*.gcno > /dev/null
llvm-cov gcov -p test/CMakeFiles/tests.dir/__/cli/src/*.gcno > /dev/null
mv *#src#core*.gcov *#cli*.gcov gcov/
rm -f *.gcov
# upload coverage report to codecov.io
bash <(curl --connect-timeout 10 --retry 5 -s https://codecov.io/bash) -X gcov -F cli

# python tests
rm -f gcov/*
lcov -q -z -d .
cd sme
python -m unittest discover -v -s ../../sme/test > sme.txt 2>&1
tail -n 100 sme.txt
cd ..
llvm-cov gcov -p src/core/CMakeFiles/core.dir/*/src/*.gcno > /dev/null
llvm-cov gcov -p sme/CMakeFiles/sme.dir/*.gcno > /dev/null
llvm-cov gcov -p sme/CMakeFiles/sme.dir/src/*.gcno > /dev/null
mv *#src#core*.gcov *#sme*.gcov gcov/
rm -f *.gcov
# upload coverage report to codecov.io
bash <(curl --connect-timeout 10 --retry 5 -s https://codecov.io/bash) -X gcov -F sme
