#!/bin/bash

# Ubuntu Sonar static analysis build script

set -e -x

# start a virtual display for the Qt GUI tests
# it can take a while before the display is ready,
# which is why it is the first thing done here
Xvfb -screen 0 1280x800x24 :99 &
export DISPLAY=:99

# download sonar scanner (generated from https://sonarcloud.io/ CI setup helper)
export SONAR_SCANNER_VERSION=4.4.0.2170
export SONAR_SCANNER_HOME=$HOME/.sonar/sonar-scanner-$SONAR_SCANNER_VERSION-linux
curl --create-dirs -sSLo $HOME/.sonar/sonar-scanner.zip https://binaries.sonarsource.com/Distribution/sonar-scanner-cli/sonar-scanner-cli-$SONAR_SCANNER_VERSION-linux.zip
unzip -o $HOME/.sonar/sonar-scanner.zip -d $HOME/.sonar/
export PATH=$SONAR_SCANNER_HOME/bin:$PATH
export SONAR_SCANNER_OPTS="-server"
curl --create-dirs -sSLo $HOME/.sonar/build-wrapper-linux-x86.zip https://sonarcloud.io/static/cpp/build-wrapper-linux-x86.zip
unzip -o $HOME/.sonar/build-wrapper-linux-x86.zip -d $HOME/.sonar/
export PATH=$HOME/.sonar/build-wrapper-linux-x86:$PATH

# do build
mkdir build
cd build
CC=clang CXX=clang++ cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="/opt/smelibs;/opt/smelibs/lib/cmake" -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld --coverage" -DCMAKE_CXX_FLAGS="--coverage" -DSME_WITH_TBB=ON  -DBoost_NO_BOOST_CMAKE=on
build-wrapper-linux-x86-64 --out-dir bw-output make -j2
ccache --show-stats

# start a window manager so the Qt GUI tests can have their focus set
# note: Xvfb can take a while to start up, which is why jwm is only called now,
# when (hopefully) the Xvfb display will be ready
jwm &

# run c++ tests and collect coverage data
mkdir gcov
lcov -q -z -d .
./test/tests -as > tests.txt 2>&1 || (tail -n 1000 tests.txt && exit 1)
tail -n 100 tests.txt
llvm-cov gcov -p src/core/CMakeFiles/core.dir/*/src/*.gcno > /dev/null
llvm-cov gcov -p test/CMakeFiles/tests.dir/__/src/core/*/src/*.gcno > /dev/null
llvm-cov gcov -p src/gui/CMakeFiles/gui.dir/*.gcno > /dev/null
llvm-cov gcov -p src/gui/CMakeFiles/gui.dir/*/*.gcno > /dev/null
llvm-cov gcov -p cli/CMakeFiles/cli.dir/src/*.gcno > /dev/null
llvm-cov gcov -p test/CMakeFiles/tests.dir/__/cli/src/*.gcno > /dev/null
mv *#src#core*.gcov *#src#gui*.gcov *#cli*.gcov gcov/
rm -f *.gcov
# also run python tests, but only copy gcov data for sme files: don't want to overwrite coverage info on core from c++ tests
lcov -q -z -d .
cd sme
python -m unittest discover -s ../../sme/test > sme.txt 2>&1
tail -n 100 sme.txt
cd ..
llvm-cov gcov -p sme/CMakeFiles/sme.dir/*.gcno > /dev/null
llvm-cov gcov -p sme/CMakeFiles/sme.dir/src/*.gcno > /dev/null
mv *#sme*.gcov gcov/
rm -f *.gcov

cd ..
# upload to sonar-scanner
sonar-scanner
