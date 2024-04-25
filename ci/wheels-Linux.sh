#!/bin/bash

# Script to build manylinux Python wheels

set -e -x

export CIBW_ENVIRONMENT='CMAKE_ARGS="-DSME_BUILD_CORE=OFF -DCMAKE_PREFIX_PATH=/opt/smelibs;/opt/smelibs/lib64/cmake"'
export CIBW_BEFORE_ALL='bash {project}/ci/wheels-manylinux.sh'

python -m pip install cibuildwheel=="$CIBUILDWHEEL_VERSION"
python -m cibuildwheel --output-dir ./artifacts/dist
