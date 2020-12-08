#!/bin/bash

# MacOS 10.15 GUI/CLI build script

set -e -x

brew install ccache

# check versions
cmake --version
g++ --version
python3 --version
ccache --zero-stats

sudo ./ci/getlibs.sh osx

# do build
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="/opt/smelibs;/opt/smelibs/lib/cmake;/opt/smelibs/dune" -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DSME_WITH_TBB=ON -DPYTHON_EXECUTABLE=/usr/local/bin/python3 -DSME_DUNE_COPASI_USE_FALLBACK_FILESYSTEM=ON
make -j2 VERBOSE=1
ccache --show-stats

# run cpp tests
./test/tests -as ~[gui] > tests.txt 2>&1
tail -n 100 tests.txt

# run python tests
cd sme
python3 -m unittest discover -s ../../sme/test -v
PYTHONPATH=`pwd` python3 ../../sme/test/sme_doctest.py -v
cd ..

# run benchmarks (~1 sec per benchmark, ~20secs total)
time ./benchmark/benchmark 1

# check dependencies of binaries
otool -L src/spatial-model-editor
otool -L cli/spatial-cli

# make dmg of binaries
mkdir spatial-model-editor
cp src/spatial-model-editor spatial-model-editor/.
hdiutil create spatial-model-editor -fs HFS+ -srcfolder spatial-model-editor
mkdir spatial-cli
cp cli/spatial-cli spatial-cli/.
hdiutil create spatial-cli -fs HFS+ -srcfolder spatial-cli

# move binaries to artefacts/
cd ..
mkdir artefacts
mv build/spatial-model-editor.dmg artefacts/
mv build/spatial-cli.dmg artefacts/
