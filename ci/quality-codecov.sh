#!/bin/bash

# Ubuntu Tests Coverage build script

set -e -x

# start a virtual display for the Qt GUI tests
# it can take a while before the display is ready,
# which is why it is the first thing done here
Xvfb -screen 0 1280x800x24 :99 &
export DISPLAY=:99

# download codecov cli
curl --connect-timeout 10 --retry 5 -Os https://cli.codecov.io/latest/linux/codecov
chmod +x codecov
sudo mv codecov /usr/bin
codecov --version

# break system gcov to prevent codecov from using it
whereis gcov
sudo mv /usr/bin/gcov /usr/bin/gcov.old

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
ninja -v
ccache --show-stats

# start a window manager so the Qt GUI tests can have their focus set
# note: Xvfb can take a while to start up, which is why jwm is only called now,
# when (hopefully) the Xvfb display will be ready
jwm &

# run tests and collect coverage data
mkdir gcov

run_gcov () {
    find . -type f -name "*.gcno" -print0 | xargs -0 llvm-cov gcov -p > /dev/null
    mv *#spatial-model-editor#*.gcov gcov/
    rm gcov/*#spatial-model-editor#ext#*.gcov
    rm -f *.gcov
    ls gcov/*.gcov
}

test_to_codecov () {
    NAME=$1
    TAGS=$2
    echo "Generating $NAME coverage with tags $TAGS..."
    rm -f gcov/*
    lcov -q -z -d .
    time ./test/tests -as "$TAGS" > "$NAME".txt 2>&1 || (tail -n 1000 "$NAME".txt && exit 1)
    tail -n 100 "$NAME".txt
    run_gcov
    # upload coverage report to codecov.io - needs to run from source directory, if it returns an error code try again a couple of times
    pushd ..
    codecov upload-process -Z -F "$NAME" -t "$CODECOV_TOKEN" || codecov upload-process -Z -F "$NAME" -t "$CODECOV_TOKEN" || codecov upload-process -Z -F "$NAME" -t "$CODECOV_TOKEN"
    popd
}

test_to_codecov "core" "~[expensive][core]"

test_to_codecov "gui" "~[expensive]~[mainwindow][gui]"

test_to_codecov "mainwindow" "~[expensive][mainwindow][gui]"

test_to_codecov "cli" "~[expensive][cli]"

# python tests
rm -f gcov/*
lcov -q -z -d .
cd sme
python -m pip install -r ../../sme/requirements.txt
python -m pip install -r ../../sme/requirements-test.txt
python -m pytest -sv ../../sme/test > sme.txt 2>&1 || (tail -n 1000 sme.txt && exit 1)
tail -n 100 sme.txt
cd ..
run_gcov
pushd ..
codecov upload-process -Z -F sme -t "$CODECOV_TOKEN" || codecov upload-process -Z -F sme -t "$CODECOV_TOKEN" || codecov upload-process -Z -F sme -t "$CODECOV_TOKEN"
popd
