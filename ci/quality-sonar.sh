#!/bin/bash

# Ubuntu Sonar static analysis build script

set -e -x

# temporary workaround for cmake 4.0 complaining about symengine min cmake version being too low:
export CMAKE_POLICY_VERSION_MINIMUM=3.5

# start a virtual display for the Qt GUI tests
# it can take a while before the display is ready,
# which is why it is the first thing done here
Xvfb -screen 0 1280x800x24 :99 &
export DISPLAY=:99

# download sonar scanner (generated from https://sonarcloud.io/ CI setup helper)
export SONAR_SCANNER_HOME=$HOME/.sonar/sonar-scanner-$SONAR_SCANNER_VERSION-linux-x64
wget https://github.com/spatial-model-editor/spatial-model-editor.github.io/releases/download/1.0.0/sonar-scanner-cli-$SONAR_SCANNER_VERSION-linux-x64.zip
unzip -o sonar-scanner-cli-$SONAR_SCANNER_VERSION-linux-x64.zip -d $HOME/.sonar/
export PATH=$SONAR_SCANNER_HOME/bin:$PATH
export SONAR_SCANNER_OPTS="-server"
curl --create-dirs -sSLo $HOME/.sonar/build-wrapper-linux-x86.zip https://sonarcloud.io/static/cpp/build-wrapper-linux-x86.zip
unzip -o $HOME/.sonar/build-wrapper-linux-x86.zip -d $HOME/.sonar/
export PATH=$HOME/.sonar/build-wrapper-linux-x86:$PATH

# do build
mkdir build
cd build
cmake .. \
    -GNinja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_PREFIX_PATH="/opt/smelibs;/opt/smelibs/lib/cmake" \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld --coverage" \
    -DCMAKE_CXX_FLAGS="--coverage -D_GLIBCXX_USE_TBB_PAR_BACKEND=0"
build-wrapper-linux-x86-64 --out-dir bw-output ninja
ccache --show-stats

# start a window manager so the Qt GUI tests can have their focus set
# note: Xvfb can take a while to start up, which is why jwm is only called now,
# when (hopefully) the Xvfb display will be ready
jwm &

# run c++ tests and collect coverage data
mkdir gcov

lcov -q -z -d .
time ./test/tests -as ~[expensive] >tests.txt 2>&1 || (tail -n 1000 tests.txt && exit 1)
tail -n 100 tests.txt
find . -type f -name "*.gcno" -print0 | xargs -0 llvm-cov gcov -p >/dev/null
ls *.gcov
mv *#spatial-model-editor#*.gcov gcov/
rm gcov/*#spatial-model-editor#ext#*.gcov
rm -f *.gcov
# also run python tests, but only copy gcov data for sme files: don't want to overwrite coverage info on core from c++ tests
lcov -q -z -d .
cd sme
python -m pip install -r ../../sme/requirements.txt
python -m pip install -r ../../sme/requirements-test.txt
python -m pytest -sv ../../sme/test >sme.txt 2>&1 || (tail -n 1000 sme.txt && exit 1)
tail -n 100 sme.txt
cd ..
find . -type f -name "*.gcno" -print0 | xargs -0 llvm-cov gcov -p >/dev/null
mv *#spatial-model-editor#sme#*.gcov gcov/
rm -f *.gcov

cd ..
# upload to sonar-scanner
sonar-scanner -X
