#!/bin/bash

# MacOS 11 GUI/CLI build script

set -e -x

brew install ccache

# check versions
cmake --version
g++ --version
python --version

ccache --max-size 400M
ccache --cleanup
ccache --zero-stats
ccache --show-stats

# do build
mkdir build
cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="/opt/smelibs;/opt/smelibs/lib/cmake" \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DSME_LOG_LEVEL=OFF \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="10.15"
make -j3 VERBOSE=1
ccache --show-stats

# run cpp tests
time ./test/tests -as ~[gui] > tests.txt 2>&1 || (tail -n 10000 tests.txt && exit 1)
tail -n 100 tests.txt

# run python tests
cd sme
python3 -m pip install -r ../../sme/requirements-test.txt
python3 -m pytest ../../sme/test -v
PYTHONPATH=`pwd` python ../../sme/test/sme_doctest.py -v
cd ..

# run benchmarks (~1 sec per benchmark, ~20secs total)
time ./benchmark/benchmark 1

# strip binaries
du -sh app/spatial-model-editor.app/Contents/MacOS/spatial-model-editor
du -sh cli/spatial-cli

strip app/spatial-model-editor.app/Contents/MacOS/spatial-model-editor
strip cli/spatial-cli

du -sh app/spatial-model-editor.app/Contents/MacOS/spatial-model-editor
du -sh cli/spatial-cli

# check dependencies of binaries
otool -L app/spatial-model-editor.app/Contents/MacOS/spatial-model-editor
otool -L cli/spatial-cli

# create iconset & copy into app
mkdir -p app/spatial-model-editor.app/Contents
mkdir -p app/spatial-model-editor.app/Contents/Resources
iconutil -c icns -o app/spatial-model-editor.app/Contents/Resources/icon.icns ../core/resources/icon.iconset

# make dmg of binaries
hdiutil create spatial-model-editor -fs HFS+ -srcfolder app/spatial-model-editor.app

mkdir spatial-cli
cp cli/spatial-cli spatial-cli/.
hdiutil create spatial-cli -fs HFS+ -srcfolder spatial-cli

# display version
./app/spatial-model-editor.app/Contents/MacOS/spatial-model-editor -v

# move binaries to artefacts/
cd ..
mkdir artefacts
mv build/spatial-model-editor.dmg artefacts/
mv build/spatial-cli.dmg artefacts/
