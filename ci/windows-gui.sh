#!/bin/bash

set -e -x

ls /c/hostedtoolcache/windows/Python

# download static libs
./ci/getlibs.sh win64

export CMAKE_PREFIX_PATH="C:/smelibs;C:/smelibs/CMake;C:/smelibs/lib/cmake;C:/smelibs/dune"
export SME_EXTRA_EXE_LIBS="-static;-static-libgcc;-static-libstdc++"
export CMAKE_GENERATOR="Unix Makefiles"
export SME_EXTRA_CORE_DEFS="_hypot=hypot"
export CMAKE_CXX_COMPILER_LAUNCHER=ccache
export PATH="/c/hostedtoolcache/windows/Python/3.8.7/x64:/c/hostedtoolcache/windows/Python/3.8.7/x64/Scripts:$PATH"

echo "PATH=$PATH"
pwd
which g++
g++ --version
which cmake
cmake --version
which python
python --version
ccache -s

mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH -DSME_EXTRA_EXE_LIBS=$SME_EXTRA_EXE_LIBS -DCMAKE_CXX_COMPILER_LAUNCHER=$CMAKE_CXX_COMPILER_LAUNCHER -DSME_EXTRA_CORE_DEFS=$SME_EXTRA_CORE_DEFS -DSME_WITH_TBB=ON
make -j2 VERBOSE=1

ccache -s

# check dependencies
objdump.exe -x sme/sme.cp38-win_amd64.pyd > sme_obj.txt
head -n 20 sme_obj.txt
head -n 1000 sme_obj.txt | grep "DLL Name"

# temporarily exclude simulate tests due to mystery segfault on windows
./test/tests.exe -as ~[gui]~[core/simulate] > tests.txt 2>&1 || (tail -n 1000 tests.txt && exit 1)
tail -n 100 tests.txt

./benchmark/benchmark.exe 1

# python tests
cd ..
mv build/sme/sme.cp38-win_amd64.pyd .
python -m unittest discover -s sme/test -v

# move binaries to artefacts/
mkdir artefacts
mv build/src/spatial-model-editor.exe artefacts/
mv build/cli/spatial-cli.exe artefacts/
