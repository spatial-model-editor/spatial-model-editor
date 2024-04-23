#!/bin/bash

# Windows GUI/CLI build script

set -e -x

PYDIR=$(ls -d /c/hostedtoolcache/windows/Python/3.12.*)
export PATH="$PYDIR/x64:$PYDIR/x64/Scripts:$PATH"
echo "PATH=$PATH"

export CMAKE_PREFIX_PATH="C:/smelibs;C:/smelibs/CMake;C:/smelibs/lib/cmake"
export SME_EXTRA_EXE_LIBS="-static;-static-libgcc;-static-libstdc++"
export CMAKE_GENERATOR="Unix Makefiles"
export SME_EXTRA_CORE_DEFS="_hypot=hypot"
export CMAKE_CXX_COMPILER_LAUNCHER=ccache

pwd
which g++
g++ --version
which cmake
cmake --version
which python
python --version

mkdir build
cd build
cmake .. \
    -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH \
    -DSME_EXTRA_EXE_LIBS=$SME_EXTRA_EXE_LIBS \
    -DCMAKE_CXX_COMPILER_LAUNCHER=$CMAKE_CXX_COMPILER_LAUNCHER \
    -DSME_LOG_LEVEL=OFF \
    -DSME_EXTRA_CORE_DEFS=$SME_EXTRA_CORE_DEFS \
    -DFREETYPE_LIBRARY_RELEASE=/c/smelibs/lib/libQt6BundledFreetype.a \
    -DFREETYPE_INCLUDE_DIR_freetype2=/c/smelibs/include/QtFreetype \
    -DFREETYPE_INCLUDE_DIR_ft2build=/c/smelibs/include/QtFreetype
ninja -v
ccache -s -v

# check dependencies
objdump.exe -x sme/sme.cp312-win_amd64.pyd > sme_obj.txt
head -n 20 sme_obj.txt
head -n 1000 sme_obj.txt | grep "DLL Name"

time ./test/tests.exe -as ~[gui] > tests.txt 2>&1 || (tail -n 1000 tests.txt && exit 1)
tail -n 100 tests.txt

./benchmark/benchmark.exe 1

# python tests
cd ..
mv build/sme/sme.cp312-win_amd64.pyd .
python -m pip install -r sme/requirements-test.txt
python -m pytest sme/test -v

# strip binaries
du -sh build/app/spatial-model-editor.exe
du -sh build/cli/spatial-cli.exe

strip build/app/spatial-model-editor.exe
strip build/cli/spatial-cli.exe

du -sh build/app/spatial-model-editor.exe
du -sh build/cli/spatial-cli.exe

# display version
./build/app/spatial-model-editor.exe -v

# move binaries to artifacts/binaries
mkdir -p artifacts/binaries
mv build/app/spatial-model-editor.exe artifacts/binaries/
mv build/cli/spatial-cli.exe artifacts/binaries/
