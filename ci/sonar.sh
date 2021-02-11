#!/bin/bash

# Ubuntu 16.04 Sonar static analysis build script

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

# start a virtual display for the Qt GUI tests
Xvfb -screen 0 1280x800x24 :99 &
export DISPLAY=:99

# start a window manager so the Qt GUI tests can have their focus set
jwm &

# do build
mkdir build
cd build
CC=clang CXX=clang++ cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="/opt/smelibs;/opt/smelibs/lib/cmake" -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=lld -DSME_WITH_TBB=ON  -DBoost_NO_BOOST_CMAKE=on
build-wrapper-linux-x86-64 --out-dir bw-output make -j2
ccache --show-stats

# run c++ tests and collect coverage data
mkdir gcov
lcov -q -z -d .
LSAN_OPTIONS=exitcode=0 ./test/tests -as > tests.txt 2>&1 || (tail -n 1000 tests.txt && exit 1)
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
LSAN_OPTIONS=exitcode=0 LD_PRELOAD=$(clang -print-file-name=libclang_rt.asan-x86_64.so) PYTHONMALLOC=malloc python -m unittest discover -s ../../sme/test > sme.txt 2>&1
tail -n 100 sme.txt
cd ..
llvm-cov gcov -p sme/CMakeFiles/sme.dir/*.gcno > /dev/null
llvm-cov gcov -p sme/CMakeFiles/sme.dir/src/*.gcno > /dev/null
mv *#sme*.gcov gcov/
rm -f *.gcov

cd ..
# upload to sonar-scanner
sonar-scanner
