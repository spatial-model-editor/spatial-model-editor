#!/bin/bash

# Windows GUI/CLI build script

set -e -x

PYDIR=$(ls -d /c/hostedtoolcache/windows/Python/3.9.*)
export PATH="$PYDIR/x64:$PYDIR/x64/Scripts:$PATH"
echo "PATH=$PATH"

export CMAKE_PREFIX_PATH="C:/smelibs;C:/smelibs/CMake;C:/smelibs/lib/cmake"
export SME_EXTRA_EXE_LIBS="-static;-static-libgcc;-static-libstdc++"
export CMAKE_GENERATOR="Unix Makefiles"
# stop Qt from defining UNICODE on windows to avoid dune issues
# used to be opt-in, done by default for Qt >= 6.1.2 see
# https://codereview.qt-project.org/c/qt/qtbase/+/350443
export SME_QT_DISABLE_UNICODE=TRUE
export SME_EXTRA_CORE_DEFS="_hypot=hypot"
export CMAKE_CXX_COMPILER_LAUNCHER=ccache

pwd
which g++
g++ --version
which cmake
cmake --version
which python
python --version

ccache --max-size 400M
ccache --cleanup
ccache --zero-stats
ccache --show-stats

mkdir build
cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH \
    -DSME_EXTRA_EXE_LIBS=$SME_EXTRA_EXE_LIBS \
    -DCMAKE_CXX_COMPILER_LAUNCHER=$CMAKE_CXX_COMPILER_LAUNCHER \
    -DSME_QT_DISABLE_UNICODE=$SME_QT_DISABLE_UNICODE \
    -DSME_EXTRA_CORE_DEFS=$SME_EXTRA_CORE_DEFS
make -j2 VERBOSE=1

ccache -s

# check dependencies
objdump.exe -x sme/sme.cp39-win_amd64.pyd > sme_obj.txt
head -n 20 sme_obj.txt
head -n 1000 sme_obj.txt | grep "DLL Name"

time ./test/tests.exe -as ~[gui] > tests.txt 2>&1 || (tail -n 1000 tests.txt && exit 1)
tail -n 100 tests.txt

./benchmark/benchmark.exe 1

# python tests
cd ..
mv build/sme/sme.cp39-win_amd64.pyd .
python -m pip install -r sme/requirements.txt
python -m unittest discover -s sme/test -v

# strip binaries
du -sh build/app/spatial-model-editor.exe
du -sh build/cli/spatial-cli.exe

strip build/app/spatial-model-editor.exe
strip build/cli/spatial-cli.exe

du -sh build/app/spatial-model-editor.exe
du -sh build/cli/spatial-cli.exe

# display version
./build/app/spatial-model-editor.exe -v

# move binaries to artefacts/
mkdir artefacts
mv build/app/spatial-model-editor.exe artefacts/
mv build/cli/spatial-cli.exe artefacts/
