#!/bin/bash

# Ubuntu Tests Coverage build script

set -e -x

# start a virtual display for the Qt GUI tests
# it can take a while before the display is ready,
# which is why it is the first thing done here
Xvfb -screen 0 1280x800x24 :99 &
export DISPLAY=:99

# download codecov uploader
curl --connect-timeout 10 --retry 5 -Os https://uploader.codecov.io/latest/linux/codecov
chmod +x codecov
sudo mv codecov /usr/bin
codecov --version

# do build
mkdir build
cd build
cmake .. \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH="/opt/smelibs;/opt/smelibs/lib/cmake" \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DCMAKE_EXE_LINKER_FLAGS="--coverage" \
  -DCMAKE_CXX_FLAGS="--coverage" \
  -DSME_WITH_TBB=ON \
  -DBoost_NO_BOOST_CMAKE=on \
  -DSTDTHREAD_WORKS=ON
make -j2 VERBOSE=1
ccache --show-stats

# start a window manager so the Qt GUI tests can have their focus set
# note: Xvfb can take a while to start up, which is why jwm is only called now,
# when (hopefully) the Xvfb display will be ready
jwm &

# run tests and collect coverage data
mkdir gcov

# core unit core_tests
lcov -q -z -d .
time ./test/tests -as "~[expensive][core]" > core.txt 2>&1 || (tail -n 1000 core.txt && exit 1)
tail -n 100 core.txt
gcov -p src/core/CMakeFiles/core.dir/*/src/*.gcno > /dev/null
gcov -p test/CMakeFiles/tests.dir/__/src/core/*/src/*.gcno > /dev/null
mv *#src#core*.gcov gcov/
rm -f *.gcov
ls gcov
# upload coverage report to codecov.io
codecov -X gcov -F core

# gui unit tests
rm -f gcov/*
lcov -q -z -d .
time ./test/tests -as "~[expensive]~[mainwindow][gui]" > gui.txt 2>&1 || (tail -n 1000 gui.txt && exit 1)
tail -n 100 gui.txt
gcov -p src/core/CMakeFiles/core.dir/*/src/*.gcno > /dev/null
gcov -p test/CMakeFiles/tests.dir/__/src/core/*/src/*.gcno > /dev/null
gcov -p src/gui/CMakeFiles/gui.dir/*.gcno > /dev/null
gcov -p src/gui/CMakeFiles/gui.dir/*/*.gcno > /dev/null
mv *#src#core*.gcov *#src#gui*.gcov gcov/
rm -f *.gcov
# upload coverage report to codecov.io
codecov -X gcov -F gui

# mainwindow unit tests
rm -f gcov/*
lcov -q -z -d .
time ./test/tests -as "~[expensive][mainwindow][gui]" > gui-mainwindow.txt 2>&1 || (tail -n 1000 gui-mainwindow.txt && exit 1)
tail -n 100 gui-mainwindow.txt
gcov -p src/core/CMakeFiles/core.dir/*/src/*.gcno > /dev/null
gcov -p test/CMakeFiles/tests.dir/__/src/core/*/src/*.gcno > /dev/null
gcov -p src/gui/CMakeFiles/gui.dir/*.gcno > /dev/null
gcov -p src/gui/CMakeFiles/gui.dir/*/*.gcno > /dev/null
mv *#src#core*.gcov *#src#gui*.gcov gcov/
rm -f *.gcov
# upload coverage report to codecov.io
codecov -X gcov -F mainwindow

# cli unit tests
rm -f gcov/*
lcov -q -z -d .
time ./test/tests -as "~[expensive][cli]" > cli.txt 2>&1 || (tail -n 1000 cli.txt && exit 1)
tail -n 100 cli.txt
gcov -p src/core/CMakeFiles/core.dir/*/src/*.gcno > /dev/null
gcov -p cli/CMakeFiles/cli.dir/src/*.gcno > /dev/null
gcov -p test/CMakeFiles/tests.dir/__/src/core/*/src/*.gcno > /dev/null
gcov -p test/CMakeFiles/tests.dir/__/cli/src/*.gcno > /dev/null
mv *#src#core*.gcov *#cli*.gcov gcov/
rm -f *.gcov
# upload coverage report to codecov.io
codecov -X gcov -F cli

# python tests
rm -f gcov/*
lcov -q -z -d .
cd sme
python -m pip install -r ../../sme/requirements.txt
python -m unittest discover -v -s ../../sme/test > sme.txt 2>&1
tail -n 100 sme.txt
cd ..
gcov -p src/core/CMakeFiles/core.dir/*/src/*.gcno > /dev/null
gcov -p sme/CMakeFiles/sme.dir/*.gcno > /dev/null
gcov -p sme/CMakeFiles/sme.dir/src/*.gcno > /dev/null
mv *#src#core*.gcov *#sme*.gcov gcov/
rm -f *.gcov
# upload coverage report to codecov.io
codecov -X gcov -F sme
