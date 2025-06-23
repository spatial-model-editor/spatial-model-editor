#!/bin/bash

# Script to build manylinux Python wheels

set -e -x

# CMAKE_POLICY_VERSION_MINIMUM is a temporary workaround for cmake 4.0 complaining about symengine min cmake version being too low:
export CIBW_ENVIRONMENT='CMAKE_POLICY_VERSION_MINIMUM=3.5 CMAKE_ARGS="-DSME_BUILD_CORE=OFF -DCMAKE_PREFIX_PATH=/opt/smelibs;/opt/smelibs/lib64/cmake" CCACHE_DIR=/host/home/runner/.cache/ccache'
export CIBW_BEFORE_ALL='bash {project}/ci/wheels-manylinux.sh'

python -m pip install cibuildwheel=="$CIBUILDWHEEL_VERSION"
python -m cibuildwheel --output-dir ./artifacts/dist
