#!/bin/bash

export CMAKE_PREFIX_PATH="C:/smelibs;C:/smelibs/CMake;C:/smelibs/lib/cmake;C:/smelibs/dune"
export SME_EXTRA_EXE_LIBS="-static;-static-libgcc;-static-libstdc++"
export CMAKE_GENERATOR="Unix Makefiles"
export SME_EXTRA_CORE_DEFS="_hypot=hypot"
export CMAKE_CXX_COMPILER_LAUNCHER=ccache
export PATH="/c/Python38:/c/Python38/Scripts:$PATH"

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

objdump.exe -x sme/sme.cp38-win_amd64.pyd > sme_obj.txt
head -n 20 sme_obj.txt
head -n 1000 sme_obj.txt | grep "DLL Name"

./test/tests.exe -as "~[gui]" > tests.txt 2>&1
tail -n 100 tests.txt

./benchmark/benchmark.exe 1

cd ..
mv build/sme/sme.cp38-win_amd64.pyd .

python -m unittest discover -v
