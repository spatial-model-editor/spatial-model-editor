#!/bin/bash

# Ubuntu GUI/CLI build script

set -e -x

# temporary workaround for cmake 4.0 complaining about symengine min cmake version being too low:
export CMAKE_POLICY_VERSION_MINIMUM=3.5

# start a virtual display for the Qt GUI tests
# it can take a while before the display is ready,
# which is why it is the first thing done here
Xvfb -screen 0 1280x800x24 :99 &
export DISPLAY=:99

# static link stdc++ for portable binary
export SME_EXTRA_EXE_LIBS="-static-libgcc;-static-libstdc++"
# add -no-pie to avoid gcc creating a pie - it works fine when ran from the command line
# but appears as a "shared library" and can't be executed by double-clicking on it, which is not very user friendly
# see https://stackoverflow.com/a/34522357/6465472
export SME_EXTRA_GUI_LIBS="-no-pie"

# do build
mkdir build
cd build
cmake .. \
    -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="/opt/smelibs;/opt/smelibs/lib/cmake" \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DCMAKE_CXX_FLAGS="-fpic -fvisibility=hidden -D_GLIBCXX_USE_TBB_PAR_BACKEND=0" \
    -DSME_EXTRA_EXE_LIBS=$SME_EXTRA_EXE_LIBS \
    -DSME_EXTRA_GUI_LIBS=$SME_EXTRA_GUI_LIBS \
    -DSME_LOG_LEVEL=OFF \
    -DOpenGL_GL_PREFERENCE=LEGACY
ninja -v
ccache --show-stats

# start a window manager so the Qt GUI tests can have their focus set
# note: Xvfb can take a while to start up, which is why jwm is only called now,
# when (hopefully) the Xvfb display will be ready
jwm &

# run cpp tests
time ./test/tests -as >tests.txt 2>&1 || (tail -n 1000 tests.txt && exit 1)
tail -n 100 tests.txt

# run python tests
cd sme
python -m pip install -r ../../sme/requirements-test.txt
python -m pytest ../../sme/test -v
# disable doctest temporarily due to segfault in CI:
#PYTHONPATH=$(pwd) python ../../sme/test/sme_doctest.py -v
cd ..

# run benchmarks (~1 sec per benchmark, ~20secs total)
time ./benchmark/benchmark 1

# strip binaries
du -sh app/spatial-model-editor
du -sh cli/spatial-cli

strip app/spatial-model-editor
strip cli/spatial-cli

du -sh app/spatial-model-editor
du -sh cli/spatial-cli

# check dependencies of binaries
ldd app/spatial-model-editor
ldd cli/spatial-cli

# display version
./app/spatial-model-editor -v

# move binaries to artifacts/binaries
cd ..
mkdir -p artifacts/binaries
mv build/app/spatial-model-editor artifacts/binaries/spatial-model-editor-${RUNNER_ARCH}
mv build/cli/spatial-cli artifacts/binaries/spatial-cli-${RUNNER_ARCH}
